#include "renderer.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <psapi.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <filesystem>

using Microsoft::WRL::ComPtr;
static int checks=0;
static void Check(bool ok,const char* message) {
    ++checks;if (!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
static SIZE_T PrivateBytes() {
    PROCESS_MEMORY_COUNTERS_EX p{};p.cb=sizeof(p);
    GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&p),sizeof(p));
    return p.PrivateUsage;
}
static std::vector<DWORD> Pixels(IDirect3DDevice9* d) {
    ComPtr<IDirect3DSurface9> rt,copy;
    Check(SUCCEEDED(d->GetRenderTarget(0,&rt)),"effect render target");
    D3DSURFACE_DESC desc{};rt->GetDesc(&desc);
    Check(SUCCEEDED(d->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&copy,nullptr)),"effect readback surface");
    Check(SUCCEEDED(d->GetRenderTargetData(rt.Get(),copy.Get())),"effect pixel readback");
    D3DLOCKED_RECT lock{};Check(SUCCEEDED(copy->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"effect pixel lock");
    std::vector<DWORD> result(size_t(desc.Width)*desc.Height);
    for (UINT y=0;y<desc.Height;++y) {
        const auto row=reinterpret_cast<const DWORD*>(static_cast<const BYTE*>(lock.pBits)+size_t(y)*lock.Pitch);
        for (UINT x=0;x<desc.Width;++x) result[size_t(y)*desc.Width+x]=row[x]&0xFFFFFF;
    }
    copy->UnlockRect();return result;
}
static void Capture(IDirect3DDevice9* d,const wchar_t* path) {
    ComPtr<IDirect3DSurface9> rt,copy;
    Check(SUCCEEDED(d->GetRenderTarget(0,&rt)),"render target");
    D3DSURFACE_DESC desc{};rt->GetDesc(&desc);
    Check(SUCCEEDED(d->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&copy,nullptr)),"capture surface");
    Check(SUCCEEDED(d->GetRenderTargetData(rt.Get(),copy.Get())),"GPU readback");
    D3DLOCKED_RECT lock{};Check(SUCCEEDED(copy->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"capture lock");
    ComPtr<IWICImagingFactory> f;ComPtr<IWICStream> stream;ComPtr<IWICBitmapEncoder> enc;ComPtr<IWICBitmapFrameEncode> frame;
    Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))),"WIC factory");
    Check(SUCCEEDED(f->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromFilename(path,GENERIC_WRITE)),"PNG output");
    Check(SUCCEEDED(f->CreateEncoder(GUID_ContainerFormatPng,nullptr,&enc)) && SUCCEEDED(enc->Initialize(stream.Get(),WICBitmapEncoderNoCache)),"PNG encoder");
    Check(SUCCEEDED(enc->CreateNewFrame(&frame,nullptr)) && SUCCEEDED(frame->Initialize(nullptr)),"PNG frame");
    frame->SetSize(desc.Width,desc.Height);
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;
    Check(SUCCEEDED(frame->SetPixelFormat(&format)) && format==GUID_WICPixelFormat32bppBGRA,"PNG format");
    std::vector<DWORD> output(size_t(desc.Width)*desc.Height);
    size_t changed=0;
    for (UINT y=0;y<desc.Height;++y) {
        const auto row=reinterpret_cast<const DWORD*>(static_cast<const BYTE*>(lock.pBits)+size_t(y)*lock.Pitch);
        for (UINT x=0;x<desc.Width;++x) {
            if ((row[x]&0xFFFFFF)!=0x34495E) ++changed;
            output[size_t(y)*desc.Width+x]=row[x]|0xFF000000;
        }
    }
    Check(changed>200,"overlay produced visible pixels");
    Check(SUCCEEDED(frame->WritePixels(desc.Height,desc.Width*4,desc.Width*desc.Height*4,reinterpret_cast<BYTE*>(output.data()))),"PNG pixels");
    Check(SUCCEEDED(frame->Commit()) && SUCCEEDED(enc->Commit()),"PNG commit");
    copy->UnlockRect();
}
static void CheckYellowLogo(IDirect3DDevice9* device,const bodycam::Config& config,int width,int height,bool visible) {
    const auto pixels=Pixels(device);
    const auto layout=bodycam::MakeLayout(config,width,height);
    const int left=layout.x+layout.width-layout.pad-layout.logo;
    const int top=layout.y+layout.pad+layout.line/2;
    size_t yellow=0;
    for (int y=top;y<top+layout.logo;++y) for (int x=left;x<left+layout.logo;++x) {
        const DWORD pixel=pixels[size_t(y)*width+x];
        if (((pixel>>16)&255)>120 && ((pixel>>8)&255)>100 && (pixel&255)<95) ++yellow;
    }
    Check(visible?yellow>20:yellow==0,visible?"embedded yellow logo is rendered from the ASI":"ShowLogo=0 hides the yellow logo");
}
int main() {
    using namespace bodycam;
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    const auto binary=std::filesystem::path(BODYCAM_TEST_BINARY);
    const auto module=LoadLibraryW(binary.c_str());
    Check(module!=nullptr,"ASI loads in unsupported test host without patching it");
    const auto initialize=reinterpret_cast<void(*)()>(GetProcAddress(module,"InitializeASI"));
    Check(initialize!=nullptr,"ASI initialization export");initialize();
    const auto logoResource=FindResourceW(module,MAKEINTRESOURCEW(101),RT_RCDATA);
    Check(logoResource && SizeofResource(module,logoResource)>0,"downloadable ASI contains its logo resource");
    std::ifstream log(binary.parent_path()/"Bodycam.log");
    std::string logText((std::istreambuf_iterator<char>(log)),std::istreambuf_iterator<char>());
    Check(logText.find("Unsupported host")!=std::string::npos,"unsupported host guard");
    const auto malformed=ParseConfig("Scale=nan\nFontSize=-2\nMarginTop=99999\nOpacity=999\nToggleKey=oops\nOfficer=José Álvarez\nUseUTC=1\nCameraId=TEST-01\n");
    Check(malformed.scale==1.0 && malformed.fontSize==12 && malformed.marginTop==1080 && malformed.opacity==255 && malformed.toggleKey=='B',"invalid settings bounded");
    Check(malformed.officer==L"José Álvarez" && malformed.utc && malformed.cameraId==L"TEST-01","UTF-8 and labels");
    const auto badLabel=ParseConfig("Officer="+std::string(10000,'X'));
    Check(badLabel.officer.size()==48,"label length bound");
    const Config defaults;
    const auto shipped=LoadConfig(std::filesystem::path(BODYCAM_TEST_INI).wstring());
    Check(shipped.cameraFov==defaults.cameraFov && shipped.cameraNear==defaults.cameraNear && shipped.cameraYawLimit==defaults.cameraYawLimit &&
        shipped.chestForward==defaults.chestForward && shipped.chestSide==defaults.chestSide &&
        shipped.chestHeight==defaults.chestHeight && shipped.aimForwardOffset==defaults.aimForwardOffset &&
        shipped.aimHeightOffset==defaults.aimHeightOffset && shipped.cameraId==defaults.cameraId && shipped.cameraLabel==defaults.cameraLabel,
        "shipped INI and compiled camera defaults agree");
    Check(defaults.cameraFov==100 && defaults.marginRight==260 && defaults.recordingEffect,"requested defaults");
    Check(CameraLine(ParseConfig("CameraId=TEST-01"))==L"AXON BODY 2  TEST-01","old INI uses default camera label");
    Check(CameraLine(ParseConfig("CameraLabel=CÁMARA LOCAL\nCameraId="))==L"CÁMARA LOCAL","UTF-8 camera label without ID");
    Check(CameraLine(ParseConfig("CameraLabel=\nCameraId=TEST-01"))==L"TEST-01","empty camera label leaves ID without padding");
    Check(ParseConfig("CameraLabel="+std::string(100,'X')).cameraLabel.size()==48,"camera label length bounded");
    const auto effectOff=ParseConfig("RecordingEffect=0\nEffectIntensity=0.75\n");
    Check(!effectOff.recordingEffect && effectOff.effectIntensity==0.75,"effect switch parsed independently");
    Check(ParseConfig("EffectIntensity=nan\n").effectIntensity==0.25 && ParseConfig("EffectIntensity=100\n").effectIntensity==1 && ParseConfig("EffectIntensity=-1\n").effectIntensity==0,"effect intensity bounded");
    for (const auto size : {std::pair<int,int>{1920,1080},{3840,2160}}) {
        const auto l=MakeLayout(defaults,size.first,size.second);
        Check(size.first-l.x-l.width==260*(size.second/1080),"default right HUD clearance scales with resolution");
    }
    SYSTEMTIME t{};t.wYear=2026;t.wMonth=10;t.wDay=2;t.wHour=23;t.wMinute=7;t.wSecond=4;
    Check(Timestamp(t,false,-210)==L"2026-10-02 23:07:04 -0330","negative time offset");
    Check(Timestamp(t,true,0)==L"2026-10-02 23:07:04 Z","UTC timestamp");
    for (const auto size : {std::pair<int,int>{640,480},{1920,1080},{3840,2160},{3440,1440}}) {
        const auto l=MakeLayout(malformed,size.first,size.second);
        Check(l.x>=0 && l.y>=0 && l.x+l.width<=size.first && l.y+l.height<=size.second,"layout within screen");
    }
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"BodycamHiddenValidation";
    RegisterClassW(&wc);
    HWND window=CreateWindowW(wc.lpszClassName,L"Bodycam test",WS_OVERLAPPED,0,0,1920,1080,nullptr,nullptr,wc.hInstance,nullptr);
    Check(window!=nullptr,"hidden test window");
    ComPtr<IDirect3D9> api;api.Attach(Direct3DCreate9(D3D_SDK_VERSION));Check(api!=nullptr,"D3D9 available");
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
    pp.BackBufferWidth=1920;pp.BackBufferHeight=1080;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    ComPtr<IDirect3DDevice9> device;
    Check(SUCCEEDED(api->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&device)),"hardware D3D9 device");
    // Warm up Windows font/WIC caches before checking for per-lifecycle handle leaks.
    { Renderer warm;Config c;device->BeginScene();Check(warm.Draw(device.Get(),c,module),"warmup draw");device->EndScene();warm.Release(); }
    const DWORD gdiBefore=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    {
        Renderer renderer;Config c;c.officer=L"AGENTE";c.badge=L"0000";
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF34495E,1,0);
        Check(SUCCEEDED(device->BeginScene()),"begin scene");
        device->SetRenderState(D3DRS_FOGENABLE,TRUE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
        device->SetRenderState(D3DRS_COLORWRITEENABLE,7);device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
        ComPtr<IDirect3DVertexBuffer9> vb;
        Check(SUCCEEDED(device->CreateVertexBuffer(256,0,D3DFVF_XYZ,D3DPOOL_MANAGED,&vb,nullptr)),"sentinel stream");
        device->SetFVF(D3DFVF_XYZ);device->SetStreamSource(0,vb.Get(),0,12);
        Check(renderer.Draw(device.Get(),c,module),"first draw");
        DWORD value=0;device->GetRenderState(D3DRS_FOGENABLE,&value);Check(value==TRUE,"fog restored");
        device->GetRenderState(D3DRS_ALPHABLENDENABLE,&value);Check(value==FALSE,"blending restored");
        device->GetRenderState(D3DRS_COLORWRITEENABLE,&value);Check(value==7,"color write restored");
        device->GetSamplerState(0,D3DSAMP_ADDRESSU,&value);Check(value==D3DTADDRESS_WRAP,"sampler restored");
        device->GetFVF(&value);Check(value==D3DFVF_XYZ,"vertex format restored");
        ComPtr<IDirect3DVertexBuffer9> after;UINT offset=0,stride=0;device->GetStreamSource(0,&after,&offset,&stride);
        Check(after.Get()==vb.Get() && stride==12,"stream restored after DrawPrimitiveUP");
        after.Reset();device->SetStreamSource(0,nullptr,0,0);vb.Reset();
        device->EndScene();Capture(device.Get(),L"preview-1080p.png");
        CheckYellowLogo(device.Get(),c,1920,1080,true);
        const auto firstUpdates=renderer.Updates();
        LARGE_INTEGER freq{},start{},stop{};QueryPerformanceFrequency(&freq);QueryPerformanceCounter(&start);
        const auto memBefore=PrivateBytes();
        for (int i=0;i<1200;++i) {
            device->BeginScene();Check(renderer.Draw(device.Get(),c,module),"repeated draw");device->EndScene();
        }
        QueryPerformanceCounter(&stop);
        const double ms=1000.0*(stop.QuadPart-start.QuadPart)/freq.QuadPart;
        const auto memAfter=PrivateBytes();
        Check(renderer.Updates()-firstUpdates<=static_cast<unsigned long>(ms/1000)+2,"clock updated at most once per second");
        Check(memAfter<=memBefore+16*1024*1024,"no large growth across 1200 draws");
        c.enabled=false;c.recordingEffect=false;const auto beforeHidden=renderer.Frames();
        Check(!renderer.Draw(device.Get(),c,module) && beforeHidden==renderer.Frames(),"disabled overlay and effect skip drawing");
        c.recordingEffect=true;c.effectIntensity=0;
        Check(!renderer.Draw(device.Get(),c,module),"zero effect intensity skips drawing");
        c.effectIntensity=0.25;
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF34495E,1,0);device->BeginScene();
        const auto updatesBeforeEffect=renderer.Updates();
        Check(renderer.Draw(device.Get(),c,module),"effect works with label hidden");device->EndScene();
        Check(renderer.Updates()==updatesBeforeEffect,"hidden label does not update GDI text");
        const auto affected=Pixels(device.Get());size_t changedEffect=0;unsigned maxDifference=0;
        for (DWORD pixel:affected) {
            if (pixel!=0x34495E) ++changedEffect;
            for (int shift : {0,8,16}) maxDifference=std::max(maxDifference,unsigned(std::abs(int((pixel>>shift)&255)-int((0x34495Eu>>shift)&255))));
        }
        Check(changedEffect>affected.size()/2 && maxDifference<=12,"default full-screen effect changes pixels only slightly");
        c.recordingEffect=false;c.enabled=true;
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF34495E,1,0);device->BeginScene();
        Check(renderer.Draw(device.Get(),c,module),"label remains when effect disabled");device->EndScene();
        const auto disabled=Pixels(device.Get());const auto label=MakeLayout(c,1920,1080);
        bool clean=true;
        for (int y=0;y<1080;++y) for (int x=0;x<1920;++x) {
            if ((x<label.x || x>=label.x+label.width || y<label.y || y>=label.y+label.height) && disabled[size_t(y)*1920+x]!=0x34495E) clean=false;
        }
        Check(clean,"effect disabled leaves every pixel outside label untouched");
        // Reload a label of the same length: texture dimensions cannot mask a stale title.
        c.cameraLabel=L"TEST CAM 2";
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF34495E,1,0);device->BeginScene();
        Check(renderer.Draw(device.Get(),c,module,true),"reload camera label");device->EndScene();
        const auto renamed=Pixels(device.Get());
        size_t titleChanges=0;
        for (int y=label.y+label.pad+label.line;y<label.y+label.pad+2*label.line;++y)
            for (int x=label.x;x<label.x+label.width;++x)
                if (renamed[size_t(y)*1920+x]!=disabled[size_t(y)*1920+x]) ++titleChanges;
        Check(titleChanges>20,"config reload replaces actual rendered camera title");
        c.cameraLabel=Config{}.cameraLabel;
        c.showLogo=false;
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF34495E,1,0);device->BeginScene();
        Check(renderer.Draw(device.Get(),c,module,true),"label draws with logo disabled");device->EndScene();
        CheckYellowLogo(device.Get(),c,1920,1080,false);
        c.showLogo=true;
        c.recordingEffect=true;
        // Real Reset with the overlay's managed textures still alive.
        pp.BackBufferWidth=3840;pp.BackBufferHeight=2160;
        Check(SUCCEEDED(device->Reset(&pp)),"reset to 4K with existing resources");
        device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF34495E,1,0);device->BeginScene();
        Check(renderer.Draw(device.Get(),c,module),"4K draw after reset");device->EndScene();Capture(device.Get(),L"preview-4k.png");
        CheckYellowLogo(device.Get(),c,3840,2160,true);
        // Isolated 4K workload with a GPU fence at both ends, not a GTA FPS claim.
        ComPtr<IDirect3DQuery9> fence;
        Check(SUCCEEDED(device->CreateQuery(D3DQUERYTYPE_EVENT,&fence)),"GPU completion query");
        const auto drain=[&] {
            Check(SUCCEEDED(fence->Issue(D3DISSUE_END)),"GPU completion issued");
            const auto deadline=GetTickCount64()+5000;
            HRESULT status;
            while ((status=fence->GetData(nullptr,0,D3DGETDATA_FLUSH))==S_FALSE && GetTickCount64()<deadline) Sleep(0);
            Check(status==S_OK,"GPU completed within timeout");
        };
        double cost4k[2]{};
        for (int effect=0;effect<2;++effect) {
            c.recordingEffect=effect!=0;drain();QueryPerformanceCounter(&start);
            for (int frame=0;frame<64;++frame) {
                device->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF34495E,1,0);device->BeginScene();
                Check(renderer.Draw(device.Get(),c,module),"4K isolated frame");device->EndScene();
            }
            drain();QueryPerformanceCounter(&stop);
            cost4k[effect]=1000.0*(stop.QuadPart-start.QuadPart)/freq.QuadPart/64;
        }
        fence.Reset();c.recordingEffect=true;
        std::ofstream effectReport("validation-effect.json");
        effectReport<<"{\"default_max_channel_change\": "<<maxDifference<<", \"disabled_pixels_untouched\": true, \"frame_ms_without_effect_4k\": "<<cost4k[0]<<", \"frame_ms_with_effect_4k\": "<<cost4k[1]<<", \"method\": \"64 isolated clear-and-overlay frames, GPU completed\", \"gameplay_validated\": false}\n";
        double maxUpdateMs=0;
        for (int n=0;n<3;++n) {
            Sleep(1005);const auto beforeUpdate=renderer.Updates();
            device->BeginScene();QueryPerformanceCounter(&start);
            Check(renderer.Draw(device.Get(),c,module),"timed 4K clock refresh");
            QueryPerformanceCounter(&stop);device->EndScene();
            maxUpdateMs=std::max(maxUpdateMs,1000.0*(stop.QuadPart-start.QuadPart)/freq.QuadPart);
            Check(renderer.Updates()==beforeUpdate+1,"one clock refresh per elapsed second");
        }
        for (int i=0;i<12;++i) {
            c.scale=(i%2)?0.8:1.2;device->BeginScene();
            Check(renderer.Draw(device.Get(),c,module,true),"config reload / resources recreated");device->EndScene();
        }
        pp.BackBufferWidth=1920;pp.BackBufferHeight=1080;
        Check(SUCCEEDED(device->Reset(&pp)),"reset back to 1080p");
        renderer.Release();
        const auto gdiAfter=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
        std::printf("GDI initial=%lu after=%lu\n",gdiBefore,gdiAfter);
        Check(gdiAfter==gdiBefore,"GDI handles released");
        std::ofstream report("validation.json");
        report<<"{\n  \"checks\": "<<checks<<",\n  \"draws\": 1200,\n  \"elapsed_ms\": "<<ms<<",\n  \"mean_cpu_submission_ms\": "<<ms/1200<<",\n  \"max_clock_refresh_cpu_ms_4k\": "<<maxUpdateMs<<",\n  \"private_bytes_before\": "<<memBefore<<",\n  \"private_bytes_after\": "<<memAfter<<",\n  \"reset_4k\": true,\n  \"state_restored\": true,\n  \"gdi_released\": true,\n  \"gameplay_validated\": false\n}\n";
    }
    device.Reset();api.Reset();DestroyWindow(window);FreeLibrary(module);CoUninitialize();
    std::printf("PASS: %d checks; PNG previews and validation.json created. Gameplay not yet tested.\n",checks);
}
