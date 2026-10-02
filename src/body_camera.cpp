#include "body_camera.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>

namespace bodycam {
namespace {
template<typename T> T Read(const void* p,size_t off) {T v{};std::memcpy(&v,static_cast<const char*>(p)+off,sizeof(v));return v;}
template<typename T> void Write(void* p,size_t off,T v) {std::memcpy(static_cast<char*>(p)+off,&v,sizeof(v));}
bool Equal(Vec3 a,Vec3 b) {return a.x==b.x && a.y==b.y && a.z==b.z;}
bool Equal(Vec2 a,Vec2 b) {return a.x==b.x && a.y==b.y;}
float LengthSquared(Vec3 v) {return v.x*v.x+v.y*v.y+v.z*v.z;}
Vec3 Unit(Vec3 v,Vec3 fallback) {
    const float n=LengthSquared(v);
    if (!Finite(v) || n<0.01f || n>4.0f) return fallback;
    const float s=1.0f/std::sqrt(n);return {v.x*s,v.y*s,v.z*s};
}
// Layouts verified against plugin-sdk: CCamera, CCam, CPlaceable, CPed and RwCamera.
constexpr size_t kMatrixPos=0x974+0x30,kGamePos=0x908,kRw=0x954;
constexpr size_t kCamSource=0x19C,kCamFov=0xB4,kRwNear=0x80,kRwWindow=0x68;
}
bool Finite(Vec3 v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
Vec3 ChestPosition(Vec3 chest,Vec3 forward,Vec3 right,Vec3 up,float front,float side,float height) {
    forward=Unit(forward,{0,1,0});right=Unit(right,{1,0,0});up=Unit(up,{0,0,1});
    return {chest.x+forward.x*front+right.x*side+up.x*height,
            chest.y+forward.y*front+right.y*side+up.y*height,
            chest.z+forward.z*front+right.z*side+up.z*height};
}
bool BodyCameraMode(unsigned mode,bool vehicle) {
    // Script cameras, spectating, death, cutscenes and telescopic sights are excluded.
    return vehicle ? (mode==18 || mode==3 || mode==22 || mode==55) : (mode==4 || mode==53);
}
bool BodyCamera::PlayerView(void* camera,const Config& c,CameraApi& api) {
    if (!c.cameraEnabled || !api.Accessible(camera,0xD78)) return false;
    if (!Read<uint8_t>(camera,0x2B) || Read<uint8_t>(camera,0x2C) || Read<uint8_t>(camera,0x2E) || Read<uint8_t>(camera,0x2F)) return false;
    const unsigned index=Read<uint8_t>(camera,0x59);
    if (index>2) return false;
    auto active=static_cast<char*>(camera)+0x174+index*0x238;
    auto ped=api.LocalPed();
    if (!api.Accessible(ped,0x79C) || !Read<void*>(ped,0x18) || !(Read<float>(ped,0x540)>0.0f)) return false;
    auto vehicle=api.Vehicle();
    if (vehicle && !c.cameraVehicles) return false;
    if (!BodyCameraMode(Read<uint16_t>(active,0xC),vehicle!=nullptr)) return false;
    // A supported mode alone is insufficient: the camera must belong to the local player.
    auto target=Read<void*>(active,0x21C);
    if (target!=ped && (!vehicle || target!=vehicle)) return false;
    return true;
}
bool BodyCamera::Apply(void* camera,float* globalFov,const Config& c,CameraApi& api) {
    if (applied_) Restore(api);
    if (!PlayerView(camera,c,api) || !api.Accessible(globalFov,4)) return false;
    auto active=static_cast<char*>(camera)+0x174+Read<uint8_t>(camera,0x59)*0x238;
    auto ped=api.LocalPed();
    auto matrix=Read<void*>(ped,0x14);
    auto rw=Read<void*>(camera,kRw);
    if (!api.Accessible(matrix,0x40) || !api.Accessible(rw,0x88)) return false;
    const auto pos=Read<Vec3>(matrix,0x30);
    const auto chest=api.Chest(ped);
    const Vec3 delta{chest.x-pos.x,chest.y-pos.y,chest.z-pos.z};
    if (!Finite(pos) || !Finite(chest) || LengthSquared(delta)>9.0f) return false;
    // MODE_AIMWEAPON lowers the head toward the raised arms. Keep the walking
    // lens position intact and use a bounded, separately adjustable aiming offset.
    const bool aiming=Read<uint16_t>(active,0xC)==53 && !api.Vehicle();
    const float front=float(std::clamp(c.chestForward+(aiming?c.aimForwardOffset:0.0),0.05,0.35));
    const float height=float(std::clamp(c.chestHeight+(aiming?c.aimHeightOffset:0.0),-0.30,0.30));
    const auto next=ChestPosition(chest,Read<Vec3>(matrix,0x10),Read<Vec3>(matrix,0),Read<Vec3>(matrix,0x20),
        front,float(c.chestSide),height);
    const float fov=*globalFov,nearClip=Read<float>(rw,kRwNear),camFov=Read<float>(active,kCamFov);
    const auto window=Read<Vec2>(rw,kRwWindow);
    if (!std::isfinite(fov) || fov<15 || fov>140 || !std::isfinite(nearClip) || nearClip<=0 || nearClip>5 ||
        !std::isfinite(window.x) || !std::isfinite(window.y) || window.x<=0 || window.y<=0 || window.x>10 || window.y>10 ||
        !std::isfinite(camFov)) return false;
    camera_=camera;rw_=rw;activeCam_=active;globalFov_=globalFov;
    matrixPos_=Read<Vec3>(camera,kMatrixPos);gamePos_=Read<Vec3>(camera,kGamePos);source_=Read<Vec3>(active,kCamSource);
    fov_=fov;near_=nearClip;camFov_=camFov;viewWindow_=window;
    written_=next;writtenFov_=float(c.cameraFov);writtenNear_=float(c.cameraNear);
    constexpr float radians=3.14159265358979323846f/360.0f;
    const float halfView=std::tan(writtenFov_*radians);
    // Absolute projection avoids compounding a projection rebuilt later by CameraSize.
    const float aspect=window.x/window.y;
    writtenWindow_=aspect>=1.0f?Vec2{halfView,halfView/aspect}:Vec2{halfView*aspect,halfView};
    applied_=true; // Set before the first write so fault recovery can restore a partial application.
    Write(camera,kMatrixPos,next);Write(camera,kGamePos,next);Write(active,kCamSource,next);
    *globalFov=writtenFov_;Write(active,kCamFov,writtenFov_);
    api.Near(rw,writtenNear_);api.Window(rw,writtenWindow_);api.Update(camera);
    return true;
}
void BodyCamera::Restore(CameraApi& api) {
    if (!applied_) return;
    applied_=false;
    if (!api.Accessible(camera_,0xD78)) return;
    // Only undo values still owned by this plugin. Respect later script/mod changes.
    if (Equal(Read<Vec3>(camera_,kMatrixPos),written_)) Write(camera_,kMatrixPos,matrixPos_);
    if (Equal(Read<Vec3>(camera_,kGamePos),written_)) Write(camera_,kGamePos,gamePos_);
    if (Equal(Read<Vec3>(activeCam_,kCamSource),written_)) Write(activeCam_,kCamSource,source_);
    if (Read<float>(activeCam_,kCamFov)==writtenFov_) Write(activeCam_,kCamFov,camFov_);
    const bool ownsFov=api.Accessible(globalFov_,4) && *globalFov_==writtenFov_;
    if (ownsFov) *globalFov_=fov_;
    if (Read<void*>(camera_,kRw)==rw_ && api.Accessible(rw_,0x88)) {
        if (Read<float>(rw_,kRwNear)==writtenNear_) api.Near(rw_,near_);
        if (ownsFov || Equal(Read<Vec2>(rw_,kRwWindow),writtenWindow_)) api.Window(rw_,viewWindow_);
    }
    if (api.Accessible(Read<void*>(camera_,kRw),0x88)) api.Update(camera_);
}
}
