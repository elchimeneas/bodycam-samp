#include "renderer.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace bodycam {
using Microsoft::WRL::ComPtr;
static unsigned Pow2(unsigned x) { unsigned n=1; while (n<x) n*=2; return n; }
void Renderer::Release() {
    if (text_) { text_->Release(); text_ = nullptr; }
    if (logo_) { logo_->Release(); logo_ = nullptr; }
    if (grain_) { grain_->Release(); grain_ = nullptr; }
    if (vignette_) { vignette_->Release(); vignette_ = nullptr; }
    if (dc_) {
        if (oldFont_) SelectObject(dc_, oldFont_);
        if (oldBitmap_) SelectObject(dc_, oldBitmap_);
    }
    if (font_) DeleteObject(font_);
    if (bitmap_) DeleteObject(bitmap_);
    if (dc_) DeleteDC(dc_);
    font_ = nullptr; bitmap_ = nullptr; dc_ = nullptr;
    oldBitmap_ = nullptr; oldFont_ = nullptr; pixels_ = nullptr;
    device_ = nullptr; width_=height_=fontSize_=0; logoAttempted_=false; lastUpdate_=0;
    composed_.clear();
}
bool Renderer::CreateText(IDirect3DDevice9* d, const Config&, const Layout& l) {
    width_ = int(Pow2(l.width)); height_ = int(Pow2(l.height)); fontSize_ = l.font;
    dc_ = CreateCompatibleDC(nullptr);
    if (!dc_) return false;
    BITMAPINFO info{};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=width_; info.bmiHeader.biHeight=-height_;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    bitmap_=CreateDIBSection(dc_, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels_), nullptr, 0);
    if (!bitmap_) return false;
    oldBitmap_=SelectObject(dc_,bitmap_);
    font_=CreateFontW(-l.font,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FIXED_PITCH | FF_MODERN,L"Consolas");
    if (!font_) return false;
    oldFont_=SelectObject(dc_,font_);
    SetBkMode(dc_,TRANSPARENT); SetTextColor(dc_,RGB(255,255,255));
    composed_.resize(size_t(width_)*height_);
    return SUCCEEDED(d->CreateTexture(width_,height_,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&text_,nullptr));
}
bool Renderer::UpdateText(const Config& c, const Layout& l) {
    GdiFlush();
    std::fill(pixels_,pixels_+size_t(width_)*height_,0u);
    const std::wstring officer=c.officer+(c.badge.empty()?L"":L"  PLACA "+c.badge);
    const std::wstring lines[]={CurrentTimestamp(c.utc),CameraLine(c),officer};
    const int right=l.width-l.pad-(c.showLogo ? l.logo+l.pad : 0);
    for (int n=0;n<(c.showOfficer ? 3 : 2);++n) {
        RECT r{l.pad,l.pad+n*l.line,right,l.pad+(n+1)*l.line};
        DrawTextW(dc_,lines[n].data(),int(lines[n].size()),&r,DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX|DT_END_ELLIPSIS);
    }
    GdiFlush();
    // A small black outline is composed into the same straight-alpha texture.
    const int radius=std::max(1,l.font/18);
    std::fill(composed_.begin(),composed_.end(),0u);
    // Spread coverage only from glyph pixels, rather than filtering the empty canvas.
    for (int y=0;y<l.height;++y) for (int x=0;x<l.width;++x) {
        const unsigned edge=(pixels_[size_t(y)*width_+x]&255)*3/4;
        if (!edge) continue;
        for (int dy=-radius;dy<=radius;++dy) for (int dx=-radius;dx<=radius;++dx) {
            const int xx=x+dx,yy=y+dy;
            if (xx>=0 && xx<width_ && yy>=0 && yy<height_) {
                auto& destination=composed_[size_t(yy)*width_+xx];destination=std::max(destination,edge);
            }
        }
    }
    for (size_t i=0;i<composed_.size();++i) {
        const unsigned white=pixels_[i]&255;
        const unsigned a=std::max(white,composed_[i]);
        const unsigned rgb=a ? std::min(255u,white*255/a) : 0;
        composed_[i]=(a<<24)|(rgb<<16)|(rgb<<8)|rgb;
    }
    D3DLOCKED_RECT lock{};
    if (FAILED(text_->LockRect(0,&lock,nullptr,0))) return false;
    for (int y=0;y<height_;++y) std::memcpy(static_cast<char*>(lock.pBits)+size_t(y)*lock.Pitch,
        composed_.data()+size_t(y)*width_,size_t(width_)*4);
    text_->UnlockRect(0); ++updates_;
    return true;
}
bool Renderer::CreateLogo(IDirect3DDevice9* d, HMODULE module) {
    logoAttempted_=true;
    const auto res=FindResourceW(module,MAKEINTRESOURCEW(101),RT_RCDATA);
    if (!res) return false;
    const auto size=SizeofResource(module,res);
    const auto data=static_cast<BYTE*>(LockResource(LoadResource(module,res)));
    if (!data || !size) return false;
    const HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    bool ok=false;
    {
        ComPtr<IWICImagingFactory> factory; ComPtr<IWICStream> stream;
        ComPtr<IWICBitmapDecoder> decoder; ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter> convert;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
            SUCCEEDED(factory->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromMemory(data,size)) &&
            SUCCEEDED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder)) &&
            SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&convert)) &&
            SUCCEEDED(convert->Initialize(frame.Get(),GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))) {
            UINT w=0,h=0; convert->GetSize(&w,&h);
            // The source PNG is unmodified. Only its logo region is drawn by the UI.
            if (w==456 && h==92) {
                std::vector<BYTE> bytes(size_t(w)*h*4);
                if (SUCCEEDED(convert->CopyPixels(nullptr,w*4,UINT(bytes.size()),bytes.data())) &&
                    SUCCEEDED(d->CreateTexture(512,128,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&logo_,nullptr))) {
                    D3DLOCKED_RECT lock{};
                    if (SUCCEEDED(logo_->LockRect(0,&lock,nullptr,0))) {
                        for (unsigned y=0;y<128;++y) {
                            auto row=static_cast<BYTE*>(lock.pBits)+size_t(y)*lock.Pitch;
                            std::memset(row,0,512*4);
                            if (y<h) std::memcpy(row,bytes.data()+size_t(y)*w*4,w*4);
                        }
                        logo_->UnlockRect(0); ok=true;
                    }
                }
            }
        }
    }
    if (SUCCEEDED(init)) CoUninitialize();
    if (!ok && logo_) { logo_->Release(); logo_=nullptr; }
    return ok;
}
struct Vertex { float x,y,z,rhw; DWORD color; float u,v; };
bool Renderer::CreateEffects(IDirect3DDevice9* d) {
    // Small managed textures, filled only on creation. Animation changes UVs;
    // it never reads back or copies the game's frame or uploads per-frame pixels.
    for (int which=0;which<2;++which) {
        auto& texture=which?vignette_:grain_;
        if (texture) continue;
        const UINT size=which?128:64;
        if (FAILED(d->CreateTexture(size,size,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr))) return false;
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0,&lock,nullptr,0))) {texture->Release();texture=nullptr;return false;}
        unsigned random=0xC0A3B29Du;
        for (UINT y=0;y<size;++y) {
            auto row=reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+size_t(y)*lock.Pitch);
            for (UINT x=0;x<size;++x) {
                if (which) {
                    const float nx=(float(x)+0.5f)*2/size-1,ny=(float(y)+0.5f)*2/size-1;
                    const float edge=std::clamp((nx*nx+ny*ny-0.30f)/1.70f,0.0f,1.0f);
                    const float smooth=edge*edge*(3-2*edge);
                    row[x]=DWORD(std::lround(36*smooth))<<24;
                } else {
                    random^=random<<13;random^=random>>17;random^=random<<5;
                    const DWORD alpha=12+(random%21);
                    row[x]=(alpha<<24)|((random&0x100)?0xFFFFFF:0);
                }
            }
        }
        texture->UnlockRect(0);
    }
    return true;
}
static HRESULT Quad(IDirect3DDevice9* d, IDirect3DTexture9* tex, float x,float y,float w,float h,DWORD color,
    float u0,float v0,float u1,float v1) {
    x-=0.5f;y-=0.5f;
    const Vertex v[]={{x,y,0,1,color,u0,v0},{x+w,y,0,1,color,u1,v0},{x,y+h,0,1,color,u0,v1},{x+w,y+h,0,1,color,u1,v1}};
    d->SetTexture(0,tex); return d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(Vertex));
}
bool Renderer::Draw(IDirect3DDevice9* d, const Config& c, HMODULE module, bool force) {
    const bool effect=c.recordingEffect && c.effectIntensity>0;
    if ((!c.enabled && !effect) || !d || d->TestCooperativeLevel()!=D3D_OK) return false;
    D3DVIEWPORT9 vp{}; if (FAILED(d->GetViewport(&vp)) || vp.Width<64 || vp.Height<64) return false;
    const auto l=MakeLayout(c,int(vp.Width),int(vp.Height));
    if (device_!=d || (c.enabled && (width_!=int(Pow2(l.width)) || height_!=int(Pow2(l.height)) || fontSize_!=l.font)) || force) {
        Release(); device_=d;
        force=true;
    }
    if (c.enabled && !text_ && !CreateText(d,c,l)) {Release();return false;}
    if (c.enabled && !logoAttempted_ && c.showLogo) CreateLogo(d,module);
    const bool effectReady=effect && CreateEffects(d);
    const auto now=GetTickCount64();
    if (c.enabled && (force || !lastUpdate_ || now-lastUpdate_>=1000)) {
        if (!UpdateText(c,l)) return false;
        lastUpdate_=now;
    }
    // A transient state block is released each draw, so it cannot retain resources across Reset.
    ComPtr<IDirect3DStateBlock9> state;
    if (FAILED(d->CreateStateBlock(D3DSBT_ALL,&state))) return false;
    if (FAILED(state->Capture())) return false;
    d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);
    d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    d->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);
    d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
    d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
    d->SetRenderState(D3DRS_CLIPPLANEENABLE,0);
    d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
    d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
    d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
    d->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
    d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    d->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
    d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    HRESULT effectResult=S_OK;
    if (effectReady) {
        const DWORD effectColor=(DWORD(std::lround(c.effectIntensity*255))<<24)|0xFFFFFF;
        effectResult=Quad(d,vignette_,float(vp.X),float(vp.Y),float(vp.Width),float(vp.Height),effectColor,0,0,1,1);
        d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
        d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
        const unsigned frame=unsigned(now/80);
        const float u=float((frame*17)%64)/64,v=float((frame*29)%64)/64;
        const float cell=std::max(1.0f,float(vp.Height)/1080.0f);
        const auto grainResult=Quad(d,grain_,float(vp.X),float(vp.Y),float(vp.Width),float(vp.Height),effectColor,
            u,v,u+vp.Width/(64*cell),v+vp.Height/(64*cell));
        if (FAILED(grainResult)) effectResult=grainResult;
        d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
        d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    }
    const DWORD color=(DWORD(c.opacity)<<24)|0xFFFFFF;
    const float x=float(vp.X+l.x),y=float(vp.Y+l.y);
    HRESULT textResult=S_OK;
    if (c.enabled) textResult=Quad(d,text_,x,y,float(l.width),float(l.height),color,0,0,float(l.width)/width_,float(l.height)/height_);
    HRESULT logoResult=S_OK;
    if (c.enabled && c.showLogo && logo_) logoResult=Quad(d,logo_,x+l.width-l.pad-l.logo,y+l.pad+float(l.line)/2,
        float(l.logo),float(l.logo),color,365.0f/512,3.0f/128,455.0f/512,91.0f/128);
    const HRESULT restored=state->Apply(); ++frames_;
    return SUCCEEDED(restored) && SUCCEEDED(textResult) && SUCCEEDED(logoResult) && SUCCEEDED(effectResult) && (!effect || effectReady) && (!c.enabled || !c.showLogo || logo_);
}
}
