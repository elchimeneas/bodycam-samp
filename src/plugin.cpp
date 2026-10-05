#include "renderer.h"
#include "body_camera.h"
#include "camera_cycle.h"
#include <algorithm>
#include <cstring>
#include <string>

namespace {
HMODULE module;
LONG initialized=0;
using VoidFn=void(__cdecl*)();
VoidFn originalHud=nullptr, originalShutdown=nullptr;
VoidFn originalCameraRaw=nullptr;
bodycam::Renderer* renderer=nullptr;
bodycam::Config config;
std::wstring iniPath, logPath;
bool loaded=false,toggleDown=false,reloadDown=false,forceUpdate=false,loggedDraw=false,failed=false;
bool cameraAvailable=false,cameraWasApplied=false,cameraAttempted=false,processingCamera=false;
constexpr uintptr_t kHud=0x53E4FF, kShutdown=0x53D910,kCameraProcess=0x53C104;
BYTE hudOriginal[5]{}, shutdownOriginal[5]{};
BYTE cameraOriginal[5]{};
bodycam::BodyCamera bodyCamera;
bodycam::CameraCycle cameraCycle;
constexpr uintptr_t cycleSites[]={0x528D25,0x528D3A,0x52816C,0x528181};
VoidFn originalCycle[4]{};
BYTE cycleOriginal[4][5]{};
void EnsureCameraHooks();
void __fastcall ProcessCamera(void* camera,void*);

class NativeCameraApi final : public bodycam::CameraApi {
public:
    bool Accessible(const void* p,size_t n) override {
        if (!p || !n) return false;
        MEMORY_BASIC_INFORMATION m{};
        if (!VirtualQuery(p,&m,sizeof(m)) || m.State!=MEM_COMMIT || (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return false;
        const auto a=reinterpret_cast<uintptr_t>(p),base=reinterpret_cast<uintptr_t>(m.BaseAddress);
        return a>=base && n<=m.RegionSize && a-base<=m.RegionSize-n;
    }
    void* LocalPed() override {return reinterpret_cast<void*(__cdecl*)(int)>(0x56E210)(-1);}
    void* Vehicle() override {return reinterpret_cast<void*(__cdecl*)(int,bool)>(0x56E0D0)(-1,false);}
    float* SpriteNearClip() override {return reinterpret_cast<float*>(0xC3EFA0);} // CDraw::ms_fNearClipZ
    bodycam::Vec3 Chest(void* ped) override {
        bodycam::Vec3 v;
        reinterpret_cast<void(__thiscall*)(void*,bodycam::Vec3&,unsigned,bool)>(0x5E4280)(ped,v,4,true);
        return v;
    }
    void Update(void* camera) override {
        reinterpret_cast<void(__thiscall*)(void*,bool)>(0x50AFA0)(camera,true);
        reinterpret_cast<void(__thiscall*)(void*,bool,bool)>(0x5150E0)(camera,false,false);
    }
    void Near(void* camera,float value) override {reinterpret_cast<void*(__cdecl*)(void*,float)>(0x7EE1D0)(camera,value);}
    void Window(void* camera,bodycam::Vec2 value) override {
        reinterpret_cast<void*(__cdecl*)(void*,const bodycam::Vec2*)>(0x7EE410)(camera,&value);
    }
} cameraApi;

void Log(const char* s, bool fresh=false) {
    HANDLE f=CreateFileW(logPath.c_str(),FILE_APPEND_DATA,FILE_SHARE_READ,nullptr,fresh?CREATE_ALWAYS:OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (f==INVALID_HANDLE_VALUE) return;
    LARGE_INTEGER size{};
    if (GetFileSizeEx(f,&size) && size.QuadPart<16384) {
        DWORD written; WriteFile(f,s,DWORD(std::strlen(s)),&written,nullptr);WriteFile(f,"\r\n",2,&written,nullptr);
    }
    CloseHandle(f);
}
bool Executable(uintptr_t a) {
    MEMORY_BASIC_INFORMATION m{};
    return VirtualQuery(reinterpret_cast<void*>(a),&m,sizeof(m)) && m.State==MEM_COMMIT &&
        !(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) && (m.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
bool SupportedHost() {
    const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (base!=0x400000) return false;
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<64 || dos->e_lfanew>0x100000) return false;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
    if (nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 || nt->OptionalHeader.SizeOfImage<0x900000) return false;
    // Version guards: US 1.0 call context and documented D3D device accessor.
    const BYTE context[]={0xB9,0x88,0x17,0xBA,0x00};
    const BYTE device[]={0x33,0xC0,0x53,0xA3,0x90,0x80,0xC9,0x00};
    return Executable(kHud) && Executable(kShutdown) && Executable(0x7F9D50) &&
        *reinterpret_cast<BYTE*>(kHud)==0xE8 && *reinterpret_cast<BYTE*>(kShutdown)==0xE8 &&
        std::memcmp(reinterpret_cast<void*>(0x53E509),context,sizeof(context))==0 &&
        std::memcmp(reinterpret_cast<void*>(0x7F9D50),device,sizeof(device))==0;
}
bool WriteCall(uintptr_t site,VoidFn next,VoidFn& previous,BYTE (&backup)[5]) {
    auto p=reinterpret_cast<BYTE*>(site);
    if (p[0]!=0xE8) return false;
    int oldOffset;std::memcpy(&oldOffset,p+1,4);
    const uintptr_t target=site+5+oldOffset;
    if (!Executable(target) || target==reinterpret_cast<uintptr_t>(next)) return false;
    DWORD protect;
    if (!VirtualProtect(p,5,PAGE_EXECUTE_READWRITE,&protect)) return false;
    std::memcpy(backup,p,5); previous=reinterpret_cast<VoidFn>(target);
    const auto delta=static_cast<int>(reinterpret_cast<uintptr_t>(next)-site-5);
    std::memcpy(p+1,&delta,4);
    DWORD ignored;VirtualProtect(p,5,protect,&ignored);FlushInstructionCache(GetCurrentProcess(),p,5);
    return true;
}
void RestoreCall(uintptr_t site,const BYTE (&backup)[5]) {
    DWORD protection;
    if (VirtualProtect(reinterpret_cast<void*>(site),5,PAGE_EXECUTE_READWRITE,&protection)) {
        std::memcpy(reinterpret_cast<void*>(site),backup,5);
        DWORD ignored;VirtualProtect(reinterpret_cast<void*>(site),5,protection,&ignored);
        FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(site),5);
    }
}
bool Foreground() {
    DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    return pid==GetCurrentProcessId();
}
bool Modifiers() {
    const int actual=((GetAsyncKeyState(VK_CONTROL)&0x8000)?1:0)|((GetAsyncKeyState(VK_SHIFT)&0x8000)?2:0)|((GetAsyncKeyState(VK_MENU)&0x8000)?4:0);
    return actual==config.modifiers;
}
void EnsureLoaded() {
    if (!loaded) {
        config=bodycam::LoadConfig(iniPath);renderer=new bodycam::Renderer;loaded=true;
        Log("Settings loaded. Camera follows GTA view cycle (V); starts in native view. Ctrl+Shift+B: label. Ctrl+Shift+R: reload.");
    }
}
void Inputs() {
    EnsureLoaded();
    const bool front=Foreground();
    const bool t=(GetAsyncKeyState(config.toggleKey)&0x8000)!=0;
    const bool r=(GetAsyncKeyState(config.reloadKey)&0x8000)!=0;
    if (front && Modifiers()) {
        if (t&&!toggleDown) { config.enabled=!config.enabled;Log(config.enabled?"Overlay enabled.":"Overlay hidden."); }
        if (r&&!reloadDown) {
            config=bodycam::LoadConfig(iniPath);
            if (!config.cameraEnabled) cameraCycle.Reset();
            forceUpdate=true;Log("Configuration reloaded.");
        }
    }
    toggleDown=t;reloadDown=r;
}
void Frame() {
    if (failed) return;
    Inputs();
    EnsureCameraHooks();
    const bool front=Foreground();
    if (!front || !cameraWasApplied || *reinterpret_cast<const BYTE*>(0xBA67A4)!=0) return;
    using GetDevice=IDirect3DDevice9*(__cdecl*)();
    auto d=reinterpret_cast<GetDevice>(0x7F9D50)();
    if (renderer->Draw(d,config,module,forceUpdate)) {
        forceUpdate=false;
        if (!loggedDraw) {Log("First frame drawn successfully. Gameplay compatibility still requires observation.");loggedDraw=true;}
    }
}
bool SafeRestoreCamera() {
    __try {bodyCamera.Restore(cameraApi);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
int SafeApplyCamera(void* camera) {
    __try {return bodyCamera.Apply(camera,reinterpret_cast<float*>(0x8D5038),config,cameraApi)?1:0;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return -1;}
}
bool SafePlayerView() {
    __try {return bodycam::BodyCamera::PlayerView(reinterpret_cast<void*>(0xB6F028),config,cameraApi);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void SyncCycleContext() {
    // Preserve selection while aiming/scope/menu suspends the custom perspective.
    // Reset it for respawn, a different vehicle, a script camera, or feature disable.
    const auto cam=reinterpret_cast<const BYTE*>(0xB6F028);
    auto ped=cameraApi.LocalPed();auto vehicle=cameraApi.Vehicle();
    const auto target=*reinterpret_cast<void* const*>(cam+0x958);
    const bool valid=config.cameraEnabled && cameraApi.Accessible(ped,0x79C) &&
        *reinterpret_cast<const float*>(static_cast<BYTE*>(ped)+0x540)>0 && cam[0x2B] && !cam[0x2C] &&
        (target==ped || (vehicle && target==vehicle)) && (!vehicle || config.cameraVehicles);
    cameraCycle.Context(reinterpret_cast<uintptr_t>(ped),reinterpret_cast<uintptr_t>(vehicle),valid);
}
bool FilterCycle(bool pressed,bool reverse) {
    if (!pressed || !processingCamera || !cameraAvailable) return pressed;
    SyncCycleContext();
    const auto cam=reinterpret_cast<const BYTE*>(0xB6F028);
    const bool vehicle=cameraApi.Vehicle()!=nullptr;
    const bool allowed=Foreground() && *reinterpret_cast<const BYTE*>(0xBA67A4)==0 &&
        *reinterpret_cast<const WORD*>(0xB73458+0x10E)==0 &&
        !cam[vehicle?0x39:0x38] && !cam[0x3D] && !cam[0x54] && !cam[0x1F] &&
        !*reinterpret_cast<const BYTE*>(0xA43088) && SafePlayerView();
    const bool selected=cameraCycle.Selected();
    const unsigned zoom=*reinterpret_cast<const unsigned*>(cam+(vehicle?0xB4:0xC8));
    const bool result=reverse?cameraCycle.Reverse(pressed,allowed):cameraCycle.Forward(pressed,zoom,allowed);
    if (selected!=cameraCycle.Selected()) Log(cameraCycle.Selected()?"V cycle: chest view selected after nearest view.":"V cycle: native view selected.");
    return result;
}
bool SafeFilterCycle(bool pressed,bool reverse) {
    __try {return FilterCycle(pressed,reverse);}
    __except(EXCEPTION_EXECUTE_HANDLER) {cameraCycle.Reset();return pressed;}
}
template<int I> bool __fastcall CycleInput(void* pad,void*) {
    const bool pressed=reinterpret_cast<bool(__thiscall*)(void*)>(originalCycle[I])(pad);
    return SafeFilterCycle(pressed,I==1 || I==3);
}
void EnsureCameraHooks() {
    if (cameraAttempted || !Foreground() || !SafePlayerView()) return;
    // SA-MP replaces this call during startup. Install only once the local
    // playable camera exists, chaining the already-installed SA-MP wrapper.
    cameraAttempted=true;
    const BYTE context[]={0xB9,0x28,0xF0,0xB6,0x00,0x74,0x07};
    if (!Executable(kCameraProcess) || std::memcmp(reinterpret_cast<void*>(kCameraProcess-7),context,sizeof(context))!=0) {
        Log("Camera call context unavailable; native views retained.");return;
    }
    const VoidFn callbacks[]={reinterpret_cast<VoidFn>(&CycleInput<0>),reinterpret_cast<VoidFn>(&CycleInput<1>),reinterpret_cast<VoidFn>(&CycleInput<2>),reinterpret_cast<VoidFn>(&CycleInput<3>)};
    for (int i=0;i<4;++i) {
        const auto site=cycleSites[i];
        // Only known native input calls are replaced; other input mods are left alone.
        const auto expected=(i==1 || i==3)?0x5404F0u:0x5404A0u;
        if (!Executable(site) || *reinterpret_cast<const BYTE*>(site)!=0xE8 ||
            site+5+*reinterpret_cast<const int*>(site+1)!=expected) {
            Log("Camera cycle input signature unavailable; native views retained.");return;
        }
    }
    int installed=0;
    for (;installed<4;++installed) {
        if (!WriteCall(cycleSites[installed],callbacks[installed],originalCycle[installed],cycleOriginal[installed])) break;
    }
    if (installed==4) cameraAvailable=WriteCall(kCameraProcess,reinterpret_cast<VoidFn>(&ProcessCamera),originalCameraRaw,cameraOriginal);
    if (!cameraAvailable) {
        while (installed) {--installed;RestoreCall(cycleSites[installed],cycleOriginal[installed]);}
        Log("Camera hook installation failed; input calls restored.");return;
    }
    Log("Camera process and native cycle calls chained after game startup.");
}
void __fastcall ProcessCamera(void* camera,void*) {
    using CameraFn=void(__thiscall*)(void*);
    if (camera!=reinterpret_cast<void*>(0xB6F028)) {reinterpret_cast<CameraFn>(originalCameraRaw)(camera);return;}
    const bool restored=SafeRestoreCamera();
    processingCamera=true;
    // Let GTA and existing hooks compute mouse input, aim and mode before relocating the lens.
    reinterpret_cast<CameraFn>(originalCameraRaw)(camera);
    processingCamera=false;
    try {Inputs();} catch (...) {config.cameraEnabled=false;Log("Camera settings failed; normal camera retained.");return;}
    if (!restored) {config.cameraEnabled=false;cameraAvailable=false;Log("Camera recovery fault; feature disabled.");return;}
    SyncCycleContext();
    int state=0;
    if (cameraAvailable && cameraCycle.Selected() && config.cameraEnabled && Foreground() && *reinterpret_cast<BYTE*>(0xBA67A4)==0) state=SafeApplyCamera(camera);
    if (state<0) {
        SafeRestoreCamera();config.cameraEnabled=false;cameraAvailable=false;
        Log("Camera access fault; normal camera requested and feature disabled.");
    } else if ((state==1)!=cameraWasApplied) {
        cameraWasApplied=state==1;
        Log(cameraWasApplied?"Chest camera applied to local player.":"Chest camera suspended; native view retained.");
    }
}
void __cdecl DrawHud() {
    originalHud();
    try { Frame(); } catch (...) {failed=true;Log("Overlay disabled after C++ error; restart to retry.");}
}
void __cdecl Shutdown() {
    SafeRestoreCamera();cameraWasApplied=false;cameraCycle.Reset();
    delete renderer;renderer=nullptr;loaded=false;loggedDraw=false;
    Log("Graphics resources released.");
    originalShutdown();
}
void Init() {
    if (InterlockedCompareExchange(&initialized,1,0)!=0) return;
    wchar_t file[32768]{};const DWORD len=GetModuleFileNameW(module,file,32768);
    if (!len || len>=32768) return;
    std::wstring dir(file,len);dir.resize(dir.find_last_of(L"\\/")+1);
    iniPath=dir+L"Bodycam.ini";logPath=dir+L"Bodycam.log";
    Log("Bodycam 0.4.6 - configurable camera label, embedded yellow logo and corrected aim/depth, Windows x86",true);
    if (!SupportedHost()) {Log("Unsupported host or modified hook signature. No hooks installed.");return;}
    if (!WriteCall(kShutdown,Shutdown,originalShutdown,shutdownOriginal)) {Log("Shutdown hook unavailable; overlay inactive.");return;}
    if (!WriteCall(kHud,DrawHud,originalHud,hudOriginal)) {
        RestoreCall(kShutdown,shutdownOriginal);Log("HUD hook unavailable; shutdown hook restored; overlay inactive.");return;
    }
    Log("Camera cycle waits for the local playable view before chaining startup hooks.");
    // The game owns the hook lifetime. Pin against unsafe manual FreeLibrary while hooks are active.
    HMODULE pinned=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&DrawHud),&pinned);
    Log("US 1.0 signatures verified; HUD and shutdown calls chained.");
}
}
extern "C" __declspec(dllexport) void InitializeASI() {Init();}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID) {
    if (reason==DLL_PROCESS_ATTACH) {module=h;DisableThreadLibraryCalls(h);Init();}
    return TRUE;
}
