#include "body_camera.h"
#include "camera_cycle.h"
#include <vector>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <fstream>
#include <algorithm>

using namespace bodycam;
static int count=0;
void Check(bool ok,const char* what) {++count;if (!ok) {std::fprintf(stderr,"FAIL camera: %s\n",what);std::exit(1);}}
template<typename T> void Put(void* p,size_t offset,T value) {std::memcpy(static_cast<char*>(p)+offset,&value,sizeof(value));}
template<typename T> T Get(const void* p,size_t offset) {T value;std::memcpy(&value,static_cast<const char*>(p)+offset,sizeof(value));return value;}
bool Close(float a,float b) {return std::abs(a-b)<0.00001f;}

struct FakeEngine : CameraApi {
    std::vector<char> camera=std::vector<char>(0xD78),ped=std::vector<char>(0x79C),matrix=std::vector<char>(0x48),rw=std::vector<char>(0x184),vehicle=std::vector<char>(0x100);
    float fov=70,spriteNear=0.9f;
    bool havePed=true,inVehicle=false,haveSpriteNear=true;
    int updates=0;
    Vec3 chest{100,200,21.5f};
    FakeEngine() {
        Put(camera.data(),0x2B,uint8_t(1));Put(camera.data(),0x59,uint8_t(0));
        Put(camera.data(),0x174+0xC,uint16_t(4));Put(camera.data(),0x174+0xB4,70.0f);
        Put(camera.data(),0x174+0x21C,static_cast<void*>(ped.data()));
        Put(camera.data(),0x954,static_cast<void*>(rw.data()));
        Put(camera.data(),0x974,Vec3{1,0,0});Put(camera.data(),0x984,Vec3{0,1,0});Put(camera.data(),0x994,Vec3{0,0,1});
        Put(camera.data(),0x174+0x190,Vec3{0,1,0});Put(camera.data(),0x174+0x1B4,Vec3{0,0,1});
        Put(camera.data(),0x174+0xBC,-1.57079632679f);Put(camera.data(),0x174+0xA0,-1.57079632679f);
        Put(camera.data(),0x974+0x30,Vec3{100,196,22});
        Put(camera.data(),0x908,Vec3{100,196,22});Put(camera.data(),0x174+0x19C,Vec3{100,196,22});
        Put(ped.data(),0x18,static_cast<void*>(ped.data()));Put(ped.data(),0x14,static_cast<void*>(matrix.data()));Put(ped.data(),0x540,100.0f);
        Put(matrix.data(),0,Vec3{1,0,0});Put(matrix.data(),0x10,Vec3{0,1,0});Put(matrix.data(),0x20,Vec3{0,0,1});Put(matrix.data(),0x30,Vec3{100,200,21});
        Put(rw.data(),0x80,0.9f);Put(rw.data(),0x68,Vec2{0.7f,0.39375f});
    }
    bool Accessible(const void* ptr,size_t n) override {
        if ((ptr==&fov || ptr==&spriteNear) && n<=sizeof(float)) return true;
        const auto p=reinterpret_cast<uintptr_t>(ptr);
        for (const auto* v:{&camera,&ped,&matrix,&rw,&vehicle}) {
            const auto b=reinterpret_cast<uintptr_t>(v->data());
            if (p>=b && n<=v->size() && p-b<=v->size()-n) return true;
        }
        return false;
    }
    void* LocalPed() override {return havePed?ped.data():nullptr;}
    void* Vehicle() override {return inVehicle?vehicle.data():nullptr;}
    float* SpriteNearClip() override {return haveSpriteNear?&spriteNear:nullptr;}
    Vec3 Chest(void*) override {return chest;}
    void Update(void*) override {++updates;}
    void Near(void* p,float v) override {Put(p,0x80,v);}
    void Window(void* p,Vec2 v) override {Put(p,0x68,v);}
};
static constexpr float pi=3.14159265358979323846f;
static Vec3 Add(Vec3 a,Vec3 b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
static Vec3 Scale(Vec3 v,float scale) {return {v.x*scale,v.y*scale,v.z*scale};}
static float Dot(Vec3 a,Vec3 b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec3 Cross(Vec3 a,Vec3 b) {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static void NativeLook(FakeEngine& api,float yaw,float pitch=0) {
    const Vec3 front{std::cos(yaw)*std::cos(pitch),std::sin(yaw)*std::cos(pitch),std::sin(pitch)};
    const Vec3 right{std::sin(yaw),-std::cos(yaw),0};
    const Vec3 up=Cross(right,front);
    Put(api.camera.data(),0x974,right);Put(api.camera.data(),0x984,front);Put(api.camera.data(),0x994,up);
    Put(api.camera.data(),0x174+0x190,front);Put(api.camera.data(),0x174+0x1B4,up);
    const float beta=std::remainder(yaw+pi,2*pi);
    Put(api.camera.data(),0x174+0xBC,beta);Put(api.camera.data(),0x174+0xA0,beta);
    Put(api.camera.data(),0x150,std::atan2(front.x,front.y));
}
static void YawTests() {
    Check(ParseConfig("").cameraYawLimit==45,"yaw limit defaults to 45 degrees per side for existing INIs");
    Check(ParseConfig("CameraYawLimit=900\n").cameraYawLimit==90 && ParseConfig("CameraYawLimit=-1\n").cameraYawLimit==10,"yaw INI cannot allow a full turn");
    Check(ParseConfig("CameraYawLimit=nan\n").cameraYawLimit==45 && ParseConfig("CameraYawLimit=60\n").cameraYawLimit==60,"yaw INI accepts a bounded angle and rejects non-finite input");
    for (unsigned mode : {4u,53u,18u}) for (float heading : {-179.0f,0.0f,90.0f,179.0f})
    for (float offset : {-170.0f,-75.0f,-30.0f,0.0f,30.0f,75.0f,170.0f}) for (float limit : {10.0f,45.0f,60.0f,90.0f}) {
        FakeEngine api;BodyCamera cam;Config config;config.cameraYawLimit=limit;
        const float body=heading*pi/180,look=(heading+offset)*pi/180;
        Put(api.matrix.data(),0x10,Vec3{std::cos(body),std::sin(body),0});
        Put(api.camera.data(),0x174+0xC,uint16_t(mode));api.inVehicle=mode==18;
        NativeLook(api,look,-0.3f);api.fov=88.55f;
        const auto pedBefore=api.ped,matrixBefore=api.matrix;
        Check(cam.Apply(api.camera.data(),&api.fov,config,api),"limited view applies on foot, aiming and in vehicles");
        const auto front=Get<Vec3>(api.camera.data(),0x984),right=Get<Vec3>(api.camera.data(),0x974),up=Get<Vec3>(api.camera.data(),0x994);
        const float relative=std::remainder(std::atan2(front.y,front.x)-body,2*pi)*180/pi;
        Check(std::abs(relative-std::clamp(offset,-limit,limit))<0.0001f,"camera stays inside torso limit across world angle wrap");
        Check(Close(front.z,std::sin(-0.3f)) && Close(Dot(front,right),0) && Close(Dot(front,up),0) && Close(Dot(up,right),0),"yaw clamp preserves pitch and orthogonal camera axes");
        const auto aimFront=Get<Vec3>(api.camera.data(),0x174+0x190),aimUp=Get<Vec3>(api.camera.data(),0x174+0x1B4);
        const float spread=std::tan(Get<float>(api.camera.data(),0x174+0xB4)*1.265f*pi/360);
        const auto ray=Add(Add(aimFront,Scale(Cross(aimFront,aimUp),spread*0.06f)),Scale(aimUp,spread*0.2f/(16.0f/9)));
        const auto window=Get<Vec2>(api.rw.data(),0x68);
        Check(Close(0.5f+Dot(ray,right)/(2*Dot(ray,front)*window.x),0.53f) && Close(0.5f-Dot(ray,up)/(2*Dot(ray,front)*window.y),0.40f),"limited weapon ray still projects onto the off-centre reticle");
        Check(api.ped==pedBefore && api.matrix==matrixBefore,"limiting view does not rotate the player's model");
        cam.Restore(api);
        Check(api.fov==88.55f && api.spriteNear==0.9f,"leaving limited view restores FOV and light depth");
    }
    FakeEngine api;BodyCamera cam;Config config;
    NativeLook(api,135*pi/180);
    for (int i=0;i<1000;++i) {
        const float previous=Get<float>(api.camera.data(),0x174+0xBC)-pi;
        NativeLook(api,previous+5*pi/180);
        Check(cam.Apply(api.camera.data(),&api.fov,config,api),"repeated outward movement applies");
        const auto front=Get<Vec3>(api.camera.data(),0x984);
        Check(std::abs(std::atan2(front.y,front.x)*180/pi-135)<0.0001f,"holding outward movement never wraps to the opposite side");
        cam.Restore(api);
    }
    NativeLook(api,Get<float>(api.camera.data(),0x174+0xBC)-pi-5*pi/180);
    Check(cam.Apply(api.camera.data(),&api.fov,config,api),"reverse movement after sustained stop");
    auto front=Get<Vec3>(api.camera.data(),0x984);
    Check(std::abs(std::atan2(front.y,front.x)*180/pi-130)<0.0001f,"reverse movement responds immediately without unwinding hidden input");
    cam.Restore(api);
    NativeLook(api,170*pi/180);Check(cam.Apply(api.camera.data(),&api.fov,config,api),"apply before later orientation change");
    NativeLook(api,pi/2);
    cam.Restore(api);
    Check(Close(Get<Vec3>(api.camera.data(),0x984).y,1) && Close(Get<Vec3>(api.camera.data(),0x174+0x190).y,1),"later camera orientation changes are respected");
}
int main() {
    YawTests();
    CameraCycle cycle;
    cycle.Context(100,0,true);
    Check(!cycle.Selected(),"starts in native view");
    // Simulate native V: far (3), middle (2), nearest (1), wrap to far.
    unsigned zoom=3;
    auto press=[&] {
        if (cycle.Forward(true,zoom,true)) zoom=zoom==1?3:zoom-1;
    };
    press();Check(zoom==2 && !cycle.Selected(),"V far to middle");
    press();Check(zoom==1 && !cycle.Selected(),"V middle to nearest");
    press();Check(zoom==1 && cycle.Selected(),"V nearest to bodycam without native zoom change");
    for (int i=0;i<100;++i) Check(!cycle.Forward(false,zoom,true) && cycle.Selected(),"holding key produces no second native edge");
    press();Check(zoom==3 && !cycle.Selected(),"next V returns to far native view");
    Check(cycle.Forward(true,1,false) && !cycle.Selected(),"disabled controls or script mode cannot select bodycam");
    Check(!cycle.Forward(false,1,true) && !cycle.Selected(),"typing V without a GTA action does nothing");
    Check(!cycle.Forward(true,1,true) && cycle.Selected(),"reselect bodycam");
    Check(!cycle.Reverse(true,true) && !cycle.Selected(),"reverse cycle leaves bodycam at nearest");
    cycle.Forward(true,1,true);cycle.Context(100,0,true);
    Check(cycle.Selected(),"selection persists in same player context");
    cycle.Context(100,200,true);Check(!cycle.Selected(),"entering vehicle resets selection");
    Check(!cycle.Forward(true,1,true) && cycle.Selected(),"vehicle nearest to bodycam");
    Check(cycle.Forward(true,1,true) && !cycle.Selected(),"vehicle next V passes to native bonnet view");
    cycle.Forward(true,1,true);cycle.Context(100,200,false);
    Check(!cycle.Selected(),"scripted camera or death resets selection");
    cycle.Context(100,0,true);cycle.Forward(true,1,true);cycle.Context(101,0,true);
    Check(!cycle.Selected(),"respawn resets selection");
    Config config;FakeEngine api;BodyCamera camera;
    const auto original=api.camera,originalRw=api.rw,originalPed=api.ped;
    Check(camera.Apply(api.camera.data(),&api.fov,config,api),"local on-foot camera enabled");
    const auto position=Get<Vec3>(api.camera.data(),0x974+0x30);
    Check(Close(position.x,100.01f)&&Close(position.y,200.20f)&&Close(position.z,21.66f),"lens follows chest plus bounded offset");
    Check(api.fov==100 && Close(Get<float>(api.rw.data(),0x80),0.04f),"default FOV 100 and near plane applied");
    Check(originalPed==api.ped,"ped and animation state not overwritten by controller");
    camera.Restore(api);
    Check(api.camera==original && api.rw==originalRw && api.fov==70 && api.spriteNear==0.9f,"all owned values restored");
    const int updates=api.updates;camera.Restore(api);Check(updates==api.updates,"restore is idempotent");
    {
        FakeEngine depthApi;BodyCamera depthCamera;Config depthConfig;
        // GTA's pretransformed light sprites use CDraw's near clip, whereas
        // world geometry uses RwCamera's projection. Reproduce the observed
        // mismatch (0.30 vs 0.04) with a light behind an opaque wall.
        const auto depth=[](double z,double nearClip,double farClip) {
            return farClip/(farClip-nearClip)*(1.0-nearClip/z);
        };
        Check(depth(100,0.30,837)<depth(30,0.04,837),"old clip mismatch incorrectly puts rear light in front of wall");
        for (float nativeNear : {0.1f,0.3f,0.9f}) for (float bodyNear : {0.04f,0.08f,0.2f}) {
            depthApi.spriteNear=nativeNear;Put(depthApi.rw.data(),0x80,nativeNear);
            depthConfig.cameraNear=bodyNear;
            Check(depthCamera.Apply(depthApi.camera.data(),&depthApi.fov,depthConfig,depthApi),"apply camera before light depth test");
            const auto geometryNear=Get<float>(depthApi.rw.data(),0x80);
            for (double farClip : {250.0,837.0,1500.0}) for (double wall : {10.0,30.0,80.0}) {
                const auto wallDepth=depth(wall,geometryNear,farClip);
                Check(depth(wall*2,depthApi.spriteNear,farClip)>wallDepth,"wall hides a light behind it in bodycam");
                Check(depth(wall/2,depthApi.spriteNear,farClip)<wallDepth,"light in front of wall remains visible");
                Check(std::abs(depth(wall,depthApi.spriteNear,farClip)-wallDepth)<1e-12,"sprites and geometry share the same depth projection");
            }
            depthCamera.Restore(depthApi);
            Check(depthApi.spriteNear==nativeNear && Get<float>(depthApi.rw.data(),0x80)==nativeNear,"normal view restores sprite and geometry clip");
        }
        Check(depthCamera.Apply(depthApi.camera.data(),&depthApi.fov,depthConfig,depthApi),"apply before independent sprite clip change");
        depthApi.spriteNear=0.5f;
        depthCamera.Restore(depthApi);
        Check(depthApi.spriteNear==0.5f && Get<float>(depthApi.rw.data(),0x80)==0.9f,"later sprite clip change respected while owned camera clip restored");
        for (float bad : {0.0f,-0.1f,6.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
            depthApi.spriteNear=bad;
            const auto beforeCamera=depthApi.camera,beforeRw=depthApi.rw;
            const auto beforeFov=depthApi.fov;
            Check(!depthCamera.Apply(depthApi.camera.data(),&depthApi.fov,depthConfig,depthApi),"invalid sprite clip prevents application");
            Check(depthApi.camera==beforeCamera && depthApi.rw==beforeRw && depthApi.fov==beforeFov,"invalid sprite clip leaves camera untouched");
        }
        depthApi.spriteNear=0.3f;depthApi.haveSpriteNear=false;
        Check(!depthCamera.Apply(depthApi.camera.data(),&depthApi.fov,depthConfig,depthApi),"missing sprite clip prevents application safely");
    }
    for (int i=0;i<1000;++i) {
        Check(camera.Apply(api.camera.data(),&api.fov,config,api),"repeat apply");
        auto window=Get<Vec2>(api.rw.data(),0x68);window.x+=0.000001f;Put(api.rw.data(),0x68,window);
        camera.Restore(api);
    }
    Check(api.camera==original && api.rw==originalRw && api.fov==70 && api.spriteNear==0.9f,"projection never accumulates across frames");
    // Regression: a widescreen fix scales the internal aim FOV by 1.265.
    // Project a point on that weapon ray through the rendered camera: it must
    // land on the same off-centre reticle for different FOVs and aspect ratios.
    for (float multiplier : {1.0f,1.265f}) for (float desired : {60.0f,100.0f,120.0f}) for (float aspect : {4.0f/3,16.0f/9,21.0f/9}) {
        FakeEngine wideApi;BodyCamera wideCamera;Config wideConfig;
        Put(wideApi.camera.data(),0x174+0xC,uint16_t(53));
        const float nativeFov=70.0f*multiplier;
        wideApi.fov=nativeFov;wideConfig.cameraFov=desired;
        Put(wideApi.rw.data(),0x68,Vec2{0.7f,0.7f/aspect});
        const auto beforeWide=wideApi.camera,beforeRw=wideApi.rw;
        for (int iteration=0;iteration<3;++iteration) {
            Check(wideCamera.Apply(wideApi.camera.data(),&wideApi.fov,wideConfig,wideApi),"widescreen aim camera applies");
            const float internalFov=Get<float>(wideApi.camera.data(),0x174+0xB4);
            const float aimSpread=std::tan(internalFov*multiplier*3.14159265358979323846f/360.0f);
            const auto projection=Get<Vec2>(wideApi.rw.data(),0x68);
            constexpr float crossX=0.53f,crossY=0.40f;
            const float rayX=(2*crossX-1)*aimSpread,rayY=(1-2*crossY)*aimSpread/aspect;
            Check(Close(0.5f+rayX/(2*projection.x),crossX) && Close(0.5f-rayY/(2*projection.y),crossY),
                "weapon ray projects onto reticle with widescreen FOV correction");
            Check(wideApi.fov==desired,"requested visual FOV preserved");
            wideCamera.Restore(wideApi);
            Check(wideApi.camera==beforeWide && wideApi.rw==beforeRw && wideApi.fov==nativeFov,"internal and rendered FOV restored independently");
        }
    }
    {
        FakeEngine other;BodyCamera owned;Config settings;
        other.fov=88.55f;
        Check(owned.Apply(other.camera.data(),&other.fov,settings,other),"apply before independent internal FOV change");
        Put(other.camera.data(),0x174+0xB4,65.0f);
        owned.Restore(other);
        Check(Get<float>(other.camera.data(),0x174+0xB4)==65 && other.fov==88.55f,"later internal FOV change retained while visual FOV restored");
        for (float bad : {0.0f,-70.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
            Put(other.camera.data(),0x174+0xB4,bad);
            const auto untouched=other.camera;
            Check(!owned.Apply(other.camera.data(),&other.fov,settings,other) && other.camera==untouched,"invalid native aim FOV rejected before writing");
        }
    }
    Check(camera.Apply(api.camera.data(),&api.fov,config,api),"apply before external camera change");
    Put(api.camera.data(),0x974+0x30,Vec3{10,20,30});api.fov=80;
    camera.Restore(api);
    Check(Get<Vec3>(api.camera.data(),0x974+0x30).x==10 && api.fov==80,"later camera/FOV changes respected");
    api.camera=original;api.rw=originalRw;api.fov=70;
    for (unsigned mode : {7u,8u,15u,16u,17u,29u,34u,39u,46u,47u,51u}) {
        Put(api.camera.data(),0x174+0xC,uint16_t(mode));const auto prior=api.camera;
        Check(!camera.Apply(api.camera.data(),&api.fov,config,api) && prior==api.camera,"special camera untouched");
    }
    Put(api.camera.data(),0x174+0xC,uint16_t(53));
    Check(camera.Apply(api.camera.data(),&api.fov,config,api),"native weapon aiming mode supported");camera.Restore(api);
    {
        FakeEngine aimApi;BodyCamera aimCamera;
        const auto tuned=ParseConfig("CameraFOV=85\nChestForward=0.08\nChestHeight=0.06\nCameraNearClip=0.04\nAimForwardOffset=0.02\nAimHeightOffset=-0.10\n");
        const auto originalAimPed=aimApi.ped;
        for (auto mode : {4u,53u,4u,53u,18u,55u}) {
            aimApi.inVehicle=mode==18 || mode==55;
            Put(aimApi.camera.data(),0x174+0xC,uint16_t(mode));
            const auto before=aimApi.camera;
            Check(aimCamera.Apply(aimApi.camera.data(),&aimApi.fov,tuned,aimApi),"walking/aiming/vehicle transition accepted");
            const auto p=Get<Vec3>(aimApi.camera.data(),0x9A4);
            Check(Close(p.y,mode==53?200.10f:200.08f) && Close(p.z,mode==53?21.46f:21.56f),"offset affects only on-foot aim; walking and vehicle position preserved");
            Check(Get<float>(aimApi.rw.data(),0x80)==0.04f && aimApi.fov==85,"aim retains near clip and FOV");
            aimCamera.Restore(aimApi);
            Check(aimApi.camera==before && aimApi.ped==originalAimPed,"aim transition restores native camera and never changes body model");
        }
        const auto bounded=ParseConfig("AimForwardOffset=99\nAimHeightOffset=-99\n");
        Check(bounded.aimForwardOffset==0.15 && bounded.aimHeightOffset==-0.20,"aim offsets bounded");
        const auto invalid=ParseConfig("AimForwardOffset=nan\nAimHeightOffset=inf\n");
        Check(invalid.aimForwardOffset==-0.06 && invalid.aimHeightOffset==-0.10,"invalid aim offsets use tuned defaults");
    }
    Put(api.camera.data(),0x174+0x21C,static_cast<void*>(api.vehicle.data()));
    Check(!camera.Apply(api.camera.data(),&api.fov,config,api),"foreign spectating target rejected");
    Put(api.camera.data(),0x174+0xC,uint16_t(18));api.inVehicle=true;
    Check(camera.Apply(api.camera.data(),&api.fov,config,api),"vehicle camera anchored to local occupant");camera.Restore(api);
    config.cameraVehicles=false;Check(!camera.Apply(api.camera.data(),&api.fov,config,api),"vehicle option honored");config.cameraVehicles=true;
    api.havePed=false;Check(!camera.Apply(api.camera.data(),&api.fov,config,api),"no player during loading");api.havePed=true;
    Put(api.ped.data(),0x540,0.0f);Check(!camera.Apply(api.camera.data(),&api.fov,config,api),"dead player excluded");Put(api.ped.data(),0x540,100.0f);
    api.chest.x=std::numeric_limits<float>::quiet_NaN();Check(!camera.Apply(api.camera.data(),&api.fov,config,api),"non-finite bones rejected");
    api.chest={100,200,400};Check(!camera.Apply(api.camera.data(),&api.fov,config,api),"outlying bone position rejected");
    api.chest={100,200,21.5f};config.cameraEnabled=false;Check(!camera.Apply(api.camera.data(),&api.fov,config,api),"normal camera switch honored");
    const auto parsed=ParseConfig("CameraFOV=nan\nCameraNearClip=-2\nChestForward=100\nCameraKey=0x43\nCameraEnabled=0\n");
    Check(parsed.cameraFov==100 && parsed.cameraNear==0.04 && parsed.chestForward==0.35 && !parsed.cameraEnabled,"camera settings bounded; old standalone CameraKey ignored");
    Check(ParseConfig("CameraFOV=999\n").cameraFov==120 && ParseConfig("CameraFOV=1\n").cameraFov==60,"user FOV range 60 to 120");
    std::ofstream out("validation-camera.json");out<<"{\"checks\": "<<count<<", \"owned_state_restored\": true, \"special_cameras_excluded\": true, \"projection_stable_1000_cycles\": true, \"gameplay_validated\": false}\n";
    std::printf("PASS camera: %d checks. Actual SA-MP view and aiming still need gameplay validation.\n",count);
}
