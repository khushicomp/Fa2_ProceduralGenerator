#include "VisualAssets.h"
#include "TextureManager.h"
#include <glad/glad.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace visual {
namespace {
TextureManager manager;
std::array<std::vector<Texture>,3> buildings;
std::vector<Texture> trees;
Texture hospital;
bool enabled=true;
std::uint32_t mix(std::uint32_t hash,float value) {
    std::uint32_t bits;std::memcpy(&bits,&value,sizeof(bits));
    return (hash^bits)*16777619u;
}
void sprite(const Texture& texture,float left,float bottom,float right,float top,bool rotate=false) {
    // Contain scaling preserves sprite aspect; it never expands a footprint.
    const float tw=static_cast<float>(rotate?texture.bottom-texture.top:texture.right-texture.left);
    const float th=static_cast<float>(rotate?texture.right-texture.left:texture.bottom-texture.top);
    const float scale=std::min((right-left)/tw,(top-bottom)/th);
    const float cx=(left+right)/2,cy=(bottom+top)/2,w=tw*scale/2,h=th*scale/2;
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_TEXTURE_BIT|GL_CURRENT_BIT);
    glEnable(GL_TEXTURE_2D);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D,texture.id);glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glColor4f(1,1,1,1);
    // WIC rows start at the top; UVs account for the Cartesian world orientation.
    const float u0=float(texture.left)/texture.width,u1=float(texture.right)/texture.width;
    const float v0=float(texture.top)/texture.height,v1=float(texture.bottom)/texture.height;
    const float uv[4][2]={{u0,v1},{u1,v1},{u1,v0},{u0,v0}};
    const float points[4][2]={{cx-w,cy-h},{cx+w,cy-h},{cx+w,cy+h},{cx-w,cy+h}};
    glBegin(GL_QUADS);
    for(int i=0;i<4;++i) {const int j=rotate?(i+1)%4:i;glTexCoord2fv(uv[j]);glVertex2fv(points[i]);}
    glEnd();glPopAttrib();
}
}
void initialize() {
    std::filesystem::path root="assets";
#ifdef _WIN32
    std::vector<wchar_t> executable(32768);
    const DWORD size=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));
    if(size && size<executable.size()) root=std::filesystem::path(executable.data()).parent_path()/"assets";
#endif
    const char* categories[]={"residential","commercial","office"};
    int loaded=0;
    for(int category=0;category<3;++category) {
        for(int i=1;i<=(category==0?6:4);++i) {
            auto t=manager.load(root/"buildings"/categories[category]/("0"+std::to_string(i)+".png"));
            buildings[category].push_back(t);if(t.id) ++loaded;
        }
    }
    hospital=manager.load(root/"buildings/hospital/01.png");if(hospital.id) ++loaded;
    for(int i=1;i<=4;++i) {auto t=manager.load(root/"vegetation/trees"/("0"+std::to_string(i)+".png"));trees.push_back(t);if(t.id)++loaded;}
    std::cout<<"Visual assets: "<<loaded<<"/19 loaded (render-only).\n";
}
void shutdown() { buildings={};trees.clear();hospital={};manager.clear(); }
void setDetailed(bool value) { enabled=value; }
bool detailed() { return enabled; }
BuildingArtwork layoutBuilding(const city::Building& b,const city::Facilities& facilities,
                              const city::Boundary& envelope) {
    const float w=b.max.x-b.min.x,h=b.max.y-b.min.y;
    BuildingArtwork result;
    result.property=envelope;
    result.bounds=envelope;
    result.category=w*h<450?0:w*h<850?1:2;
    for(const auto& facility:facilities.hospitals) {
        const auto p=facility.position;
        if(p.x>=b.min.x && p.x<=b.max.x && p.y>=b.min.y && p.y<=b.max.y) {
            result.category=3;break;
        }
    }
    const float ew=envelope.max.x-envelope.min.x,eh=envelope.max.y-envelope.min.y;
    if(ew<=0 || eh<=0) return result;
    std::uint32_t hash=2166136261u;
    for(float value:{b.min.x,b.min.y,w,h}) hash=mix(hash,value);
    struct Match { const Texture* texture;bool rotate;float error; };
    std::vector<Match> matches;
    float best=1e9f;
    auto consider=[&](const Texture& t) {
        if(!t.id) return;
        for(bool rotate:{false,true}) {
            const float aspect=rotate?float(t.bottom-t.top)/(t.right-t.left):float(t.right-t.left)/(t.bottom-t.top);
            const float error=std::abs(std::log((ew/eh)/aspect));
            matches.push_back({&t,rotate,error});best=std::min(best,error);
        }
    };
    if(result.category==3) consider(hospital);
    else for(const auto& t:buildings[result.category]) consider(t);
    matches.erase(std::remove_if(matches.begin(),matches.end(),[&](const Match& m) {
        return m.error>best+0.10f;
    }),matches.end());
    if(matches.empty()) return result;
    const auto& match=matches[hash%matches.size()];
    const auto& t=*match.texture;
    const float tw=float(match.rotate?t.bottom-t.top:t.right-t.left);
    const float th=float(match.rotate?t.right-t.left:t.bottom-t.top);
    // One complete building, never repeated roof tiles or distorted artwork.
    // Fit a compact property to the artwork, with a five-percent yard per side.
    const float scale=std::min(ew/tw,eh/th);
    const float pw=tw*scale,ph=th*scale;
    if(pw*ph<ew*eh*0.35f) return result; // Pathological slivers keep a readable fallback.
    const float cx=std::min(std::max((b.min.x+b.max.x)/2,envelope.min.x+pw/2),envelope.max.x-pw/2);
    const float cy=std::min(std::max((b.min.y+b.max.y)/2,envelope.min.y+ph/2),envelope.max.y-ph/2);
    result.property={{cx-pw/2,cy-ph/2},{cx+pw/2,cy+ph/2}};
    result.bounds={{cx-pw*0.45f,cy-ph*0.45f},{cx+pw*0.45f,cy+ph*0.45f}};
    result.texture=t;result.rotated=match.rotate;
    return result;
}
bool drawBuilding(const BuildingArtwork& artwork) {
    if(!enabled || !artwork.texture.id) return false;
    sprite(artwork.texture,artwork.bounds.min.x,artwork.bounds.min.y,
           artwork.bounds.max.x,artwork.bounds.max.y,artwork.rotated);
    return true;
}
bool building(const city::Building& b,const city::Facilities& facilities) {
    // Preserve the footprint-contained API used by asset validation and callers
    // that have not supplied a clearance-checked presentation envelope.
    return drawBuilding(layoutBuilding(b,facilities,{b.min,b.max}));
}
bool tree(float x,float y,float radius) {
    if(!enabled || trees.empty()) return false;
    const auto hash=mix(mix(2166136261u,x),y);
    const auto& t=trees[hash%trees.size()];if(!t.id) return false;
    sprite(t,x-radius,y-radius,x+radius,y+radius);return true;
}
}
