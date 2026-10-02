#include "body_camera.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <limits>

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
constexpr size_t kMatrixRight=0x974,kMatrixFront=0x984,kMatrixUp=0x994;
constexpr size_t kCamFront=0x190,kCamUp=0x1B4,kCamBeta=0xBC,kTrueBeta=0xA0,kBetaSpeed=0xC0,kOrientation=0x150;
constexpr float kPi=3.14159265358979323846f;
float Wrap(float angle) {return std::remainder(angle,2*kPi);}
Vec3 RotateYaw(Vec3 v,float angle) {
    const float cosine=std::cos(angle),sine=std::sin(angle);
    return {v.x*cosine-v.y*sine,v.x*sine+v.y*cosine,v.z};
}
}
bool Finite(Vec3 v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
float YawCorrection(Vec3 look,Vec3 bodyForward,float limitDegrees) {
    if (!Finite(look) || !Finite(bodyForward) || !std::isfinite(limitDegrees) ||
        look.x*look.x+look.y*look.y<1e-8f || bodyForward.x*bodyForward.x+bodyForward.y*bodyForward.y<1e-8f)
        return std::numeric_limits<float>::quiet_NaN();
    const float relative=Wrap(std::atan2(look.y,look.x)-std::atan2(bodyForward.y,bodyForward.x));
    const float limit=std::clamp(limitDegrees,10.0f,90.0f)*kPi/180.0f;
    return std::clamp(relative,-limit,limit)-relative;
}
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
    auto spriteNearClip=api.SpriteNearClip();
    if (!api.Accessible(spriteNearClip,sizeof(float))) return false;
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
    const float spriteNear=*spriteNearClip;
    const auto window=Read<Vec2>(rw,kRwWindow);
    if (!std::isfinite(fov) || fov<15 || fov>140 || !std::isfinite(nearClip) || nearClip<=0 || nearClip>5 ||
        !std::isfinite(window.x) || !std::isfinite(window.y) || window.x<=0 || window.y<=0 || window.x>10 || window.y>10 ||
        !std::isfinite(camFov) || camFov<15 || camFov>140 ||
        !std::isfinite(spriteNear) || spriteNear<=0 || spriteNear>5) return false;
    // CCam's FOV is an input to weapon aiming; CDraw's FOV is the rendered
    // result. Widescreen fixes can scale the former (e.g. 70 -> 88.55).
    // Preserve that native conversion instead of writing the rendered value
    // into both fields and applying the widescreen correction twice to aim.
    const float aimFov=float(c.cameraFov)*camFov/fov;
    if (!std::isfinite(aimFov) || aimFov<15 || aimFov>140) return false;
    const auto matrixRight=Read<Vec3>(camera,kMatrixRight),matrixFront=Read<Vec3>(camera,kMatrixFront),matrixUp=Read<Vec3>(camera,kMatrixUp);
    const float yaw=YawCorrection(matrixFront,Read<Vec3>(matrix,0x10),float(c.cameraYawLimit));
    if (!std::isfinite(yaw) || !Finite(matrixRight) || !Finite(matrixUp)) return false;
    const float beta=Read<float>(active,kCamBeta),trueBeta=Read<float>(active,kTrueBeta);
    if (!std::isfinite(beta) || !std::isfinite(trueBeta)) return false;
    camera_=camera;rw_=rw;activeCam_=active;globalFov_=globalFov;
    matrixPos_=Read<Vec3>(camera,kMatrixPos);gamePos_=Read<Vec3>(camera,kGamePos);source_=Read<Vec3>(active,kCamSource);
    fov_=fov;near_=nearClip;camFov_=camFov;viewWindow_=window;
    spriteNearClip_=spriteNearClip;spriteNear_=spriteNear;
    yawApplied_=std::abs(yaw)>0.000001f;
    if (yawApplied_) {
        matrixRight_=matrixRight;matrixFront_=matrixFront;matrixUp_=matrixUp;
        camFront_=Read<Vec3>(active,kCamFront);camUp_=Read<Vec3>(active,kCamUp);
        orientation_=Read<float>(camera,kOrientation);
        writtenRight_=RotateYaw(matrixRight,yaw);writtenFront_=RotateYaw(matrixFront,yaw);writtenUp_=RotateYaw(matrixUp,yaw);
        writtenOrientation_=std::atan2(writtenFront_.x,writtenFront_.y);
    }
    written_=next;writtenFov_=float(c.cameraFov);writtenCamFov_=aimFov;writtenNear_=float(c.cameraNear);
    constexpr float radians=3.14159265358979323846f/360.0f;
    const float halfView=std::tan(writtenFov_*radians);
    // Absolute projection avoids compounding a projection rebuilt later by CameraSize.
    const float aspect=window.x/window.y;
    writtenWindow_=aspect>=1.0f?Vec2{halfView,halfView/aspect}:Vec2{halfView*aspect,halfView};
    applied_=true; // Set before the first write so fault recovery can restore a partial application.
    Write(camera,kMatrixPos,next);Write(camera,kGamePos,next);Write(active,kCamSource,next);
    if (yawApplied_) {
        Write(camera,kMatrixRight,writtenRight_);Write(camera,kMatrixFront,writtenFront_);Write(camera,kMatrixUp,writtenUp_);
        // Aim rays read CCam's vectors; the renderer uses CCamera's matrix.
        // Keep both on the limited direction so bullets still follow the reticle.
        Write(active,kCamFront,writtenFront_);Write(active,kCamUp,writtenUp_);
        Write(camera,kOrientation,writtenOrientation_);
        // Consume outward input at the stop, instead of letting a hidden yaw
        // keep circling. These native input angles persist into the next frame.
        Write(active,kCamBeta,Wrap(beta+yaw));Write(active,kTrueBeta,Wrap(trueBeta+yaw));
        Write(active,kBetaSpeed,0.0f);
    }
    *globalFov=writtenFov_;Write(active,kCamFov,writtenCamFov_);
    api.Near(rw,writtenNear_);
    // GTA projects pretransformed light sprites with CDraw's cached near clip.
    // Keep it in step with RwCamera or distant halos can pass the wall's Z test.
    *spriteNearClip_=writtenNear_;
    api.Window(rw,writtenWindow_);api.Update(camera);
    return true;
}
void BodyCamera::Restore(CameraApi& api) {
    if (!applied_) return;
    applied_=false;
    if (api.Accessible(spriteNearClip_,sizeof(float)) && *spriteNearClip_==writtenNear_) *spriteNearClip_=spriteNear_;
    if (!api.Accessible(camera_,0xD78)) return;
    if (yawApplied_) {
        if (Equal(Read<Vec3>(camera_,kMatrixRight),writtenRight_) && Equal(Read<Vec3>(camera_,kMatrixFront),writtenFront_) && Equal(Read<Vec3>(camera_,kMatrixUp),writtenUp_)) {
            Write(camera_,kMatrixRight,matrixRight_);Write(camera_,kMatrixFront,matrixFront_);Write(camera_,kMatrixUp,matrixUp_);
        }
        if (Equal(Read<Vec3>(activeCam_,kCamFront),writtenFront_) && Equal(Read<Vec3>(activeCam_,kCamUp),writtenUp_)) {
            Write(activeCam_,kCamFront,camFront_);Write(activeCam_,kCamUp,camUp_);
        }
        if (Read<float>(camera_,kOrientation)==writtenOrientation_) Write(camera_,kOrientation,orientation_);
    }
    // Only undo values still owned by this plugin. Respect later script/mod changes.
    if (Equal(Read<Vec3>(camera_,kMatrixPos),written_)) Write(camera_,kMatrixPos,matrixPos_);
    if (Equal(Read<Vec3>(camera_,kGamePos),written_)) Write(camera_,kGamePos,gamePos_);
    if (Equal(Read<Vec3>(activeCam_,kCamSource),written_)) Write(activeCam_,kCamSource,source_);
    if (Read<float>(activeCam_,kCamFov)==writtenCamFov_) Write(activeCam_,kCamFov,camFov_);
    const bool ownsFov=api.Accessible(globalFov_,4) && *globalFov_==writtenFov_;
    if (ownsFov) *globalFov_=fov_;
    if (Read<void*>(camera_,kRw)==rw_ && api.Accessible(rw_,0x88)) {
        if (Read<float>(rw_,kRwNear)==writtenNear_) api.Near(rw_,near_);
        if (ownsFov || Equal(Read<Vec2>(rw_,kRwWindow),writtenWindow_)) api.Window(rw_,viewWindow_);
    }
    if (api.Accessible(Read<void*>(camera_,kRw),0x88)) api.Update(camera_);
}
}
