#pragma once
#include "config.h"
#include <d3d9.h>
#include <vector>

namespace bodycam {
class Renderer {
public:
    Renderer() = default;
    ~Renderer() { Release(); }
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    bool Draw(IDirect3DDevice9* device, const Config& config, HMODULE module, bool force = false);
    void Release();
    unsigned long Updates() const { return updates_; }
    unsigned long Frames() const { return frames_; }
private:
    bool CreateText(IDirect3DDevice9*, const Config&, const Layout&);
    bool UpdateText(const Config&, const Layout&);
    bool CreateLogo(IDirect3DDevice9*, HMODULE);
    bool CreateEffects(IDirect3DDevice9*);
    IDirect3DDevice9* device_ = nullptr; // borrowed; owned resources retain their device
    IDirect3DTexture9* text_ = nullptr;
    IDirect3DTexture9* logo_ = nullptr;
    IDirect3DTexture9* grain_ = nullptr;
    IDirect3DTexture9* vignette_ = nullptr;
    HDC dc_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HGDIOBJ oldBitmap_ = nullptr, oldFont_ = nullptr;
    HFONT font_ = nullptr;
    unsigned int* pixels_ = nullptr;
    std::vector<unsigned int> composed_;
    int width_ = 0, height_ = 0, fontSize_ = 0;
    ULONGLONG lastUpdate_ = 0;
    bool logoAttempted_ = false;
    unsigned long updates_ = 0, frames_ = 0;
};
}
