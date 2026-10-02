#pragma once
#include <cstdint>

namespace bodycam {
// Adds one view after native zoom 1. Only called for GTA's camera-cycle actions,
// never for raw keyboard input, so typing V in chat cannot select this view.
class CameraCycle {
public:
    void Context(uintptr_t ped,uintptr_t vehicle,bool valid) {
        if (!valid || ped!=ped_ || vehicle!=vehicle_) selected_=false;
        ped_=valid?ped:0;vehicle_=valid?vehicle:0;
    }
    bool Forward(bool pressed,unsigned zoom,bool allowed) {
        if (!pressed || !allowed) return pressed;
        if (selected_) {selected_=false;return true;}
        if (zoom==1) {selected_=true;return false;}
        return true;
    }
    bool Reverse(bool pressed,bool allowed) {
        if (pressed && allowed && selected_) {selected_=false;return false;}
        return pressed;
    }
    bool Selected() const {return selected_;}
    void Reset() {selected_=false;}
private:
    uintptr_t ped_=0,vehicle_=0;
    bool selected_=false;
};
}
