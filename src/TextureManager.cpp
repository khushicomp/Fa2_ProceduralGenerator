#include "TextureManager.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#endif
namespace visual {
namespace {
#ifdef _WIN32
template<class T> struct ComObject {
    T* value=nullptr;
    ~ComObject() { if(value) value->Release(); }
};
bool decode(const std::filesystem::path& path,Texture& texture,std::vector<unsigned char>& rgba) {
    const HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    struct Apartment { HRESULT result; ~Apartment(){ if(SUCCEEDED(result)) CoUninitialize(); } } apartment{init};
    if(FAILED(init) && init!=RPC_E_CHANGED_MODE) return false;
    ComObject<IWICImagingFactory> factory;
    ComObject<IWICBitmapDecoder> decoder;
    ComObject<IWICBitmapFrameDecode> frame;
    ComObject<IWICFormatConverter> converter;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,
                               IID_IWICImagingFactory,reinterpret_cast<void**>(&factory.value))) ||
       FAILED(factory.value->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,
                                  WICDecodeMetadataCacheOnLoad,&decoder.value)) ||
       FAILED(decoder.value->GetFrame(0,&frame.value)) ||
       FAILED(factory.value->CreateFormatConverter(&converter.value)) ||
       FAILED(converter.value->Initialize(frame.value,GUID_WICPixelFormat32bppRGBA,
                                WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))) return false;
    UINT w=0,h=0;
    if(FAILED(converter.value->GetSize(&w,&h)) || !w || !h || w>4096 || h>4096) return false;
    rgba.resize(static_cast<std::size_t>(w)*h*4);
    if(FAILED(converter.value->CopyPixels(nullptr,w*4,static_cast<UINT>(rgba.size()),rgba.data()))) return false;
    texture.width=static_cast<int>(w);texture.height=static_cast<int>(h);return true;
}
#else
bool decode(const std::filesystem::path&,Texture&,std::vector<unsigned char>&) { return false; }
#endif
}
TextureManager::~TextureManager() { clear(); }
const Texture& TextureManager::load(const std::filesystem::path& path) {
    auto entry=textures_.try_emplace(path);
    if(!entry.second) return entry.first->second; // Cache failures too; no per-frame disk reads.
    auto& texture=entry.first->second;
    std::vector<unsigned char> pixels;
    if(!decode(path,texture,pixels)) {
        std::cerr<<"Visual asset unavailable; using fallback: "<<path.string()<<'\n';
        return texture;
    }
    // Inspect every decoded pixel once. Rendering fits visible content, not padding.
    texture.left=texture.width;texture.top=texture.height;
    for(int y=0;y<texture.height;++y) for(int x=0;x<texture.width;++x) {
        if(pixels[(static_cast<std::size_t>(y)*texture.width+x)*4+3]) {
            texture.left=std::min(texture.left,x);texture.top=std::min(texture.top,y);
            texture.right=std::max(texture.right,x+1);texture.bottom=std::max(texture.bottom,y+1);
        }
    }
    if(texture.right<=texture.left || texture.bottom<=texture.top) return texture;
    glGenTextures(1,&texture.id);glBindTexture(GL_TEXTURE_2D,texture.id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,texture.width,texture.height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    glBindTexture(GL_TEXTURE_2D,0);
    if(glGetError()!=GL_NO_ERROR) { glDeleteTextures(1,&texture.id);texture.id=0; }
    return texture;
}
void TextureManager::clear() {
    for(auto& item:textures_) if(item.second.id) glDeleteTextures(1,&item.second.id);
    textures_.clear();
}
}
