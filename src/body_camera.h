#pragma once
#include "config.h"
#include <cstddef>

namespace bodycam {
struct Vec3 { float x=0,y=0,z=0; };
struct Vec2 { float x=0,y=0; };
bool Finite(Vec3 v);
Vec3 ChestPosition(Vec3 chest,Vec3 forward,Vec3 right,Vec3 up,float front,float side,float height);
bool BodyCameraMode(unsigned mode,bool inVehicle);

// Thin engine boundary. Tests supply allocated buffers and an isolated fake engine.
struct CameraApi {
    virtual ~CameraApi()=default;
    virtual bool Accessible(const void* address,size_t bytes)=0;
    virtual void* LocalPed()=0;
    virtual void* Vehicle()=0;
    virtual Vec3 Chest(void* ped)=0;
    virtual void Update(void* camera)=0;
    virtual void Near(void* rwCamera,float value)=0;
    virtual void Window(void* rwCamera,Vec2 value)=0;
};

class BodyCamera {
public:
    static bool PlayerView(void* camera,const Config& config,CameraApi& api);
    bool Apply(void* camera,float* globalFov,const Config& config,CameraApi& api);
    void Restore(CameraApi& api);
    bool IsApplied() const {return applied_;}
private:
    void* camera_=nullptr;
    void* rw_=nullptr;
    void* activeCam_=nullptr;
    float* globalFov_=nullptr;
    Vec3 matrixPos_{}, source_{}, gamePos_{}, written_{};
    Vec2 viewWindow_{}, writtenWindow_{};
    float fov_=0,camFov_=0,near_=0,writtenFov_=0,writtenCamFov_=0,writtenNear_=0;
    bool applied_=false;
};
}
