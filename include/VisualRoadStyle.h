#pragma once
#include "City.h"
#include <algorithm>
#include <cmath>
namespace visual {
// Rendering dimensions only. Never used to build graph edges or generated blocks.
inline float roadWidth(const city::Road& road,const std::vector<city::Road>& roads,
                       const std::vector<city::Building>& buildings) {
    const bool horizontal=road.start.y==road.end.y;
    const float coordinate=horizontal?road.start.y:road.start.x;
    float width=30;
    for(const auto& b:buildings) {
        const float low=horizontal?b.min.y:b.min.x,high=horizontal?b.max.y:b.max.x;
        const float alongLow=horizontal?b.min.x:b.min.y,alongHigh=horizontal?b.max.x:b.max.y;
        const float start=horizontal?std::min(road.start.x,road.end.x):std::min(road.start.y,road.end.y);
        const float end=horizontal?std::max(road.start.x,road.end.x):std::max(road.start.y,road.end.y);
        if(alongHigh<start || alongLow>end) continue;
        const float gap=coordinate<low?low-coordinate:coordinate>high?coordinate-high:0;
        width=std::min(width,std::max(2.0f,2*gap-1));
    }
    for(const auto& other:roads) {
        if(horizontal!=(other.start.y==other.end.y)) continue;
        const float gap=std::abs(coordinate-(horizontal?other.start.y:other.start.x));
        if(gap>0.01f) width=std::min(width,gap*0.65f);
    }
    return std::max(2.0f,width);
}
// 0 west, 1 east, 2 south, 3 north; presentation only.
inline int roadFacing(const city::Building& b,const std::vector<city::Road>& roads) {
    const city::Point c{(b.min.x+b.max.x)/2,(b.min.y+b.max.y)/2};
    float best=1e9f;int side=2;
    for(const auto& r:roads) {
        const bool horizontal=r.start.y==r.end.y;
        const float along=horizontal?c.x:c.y;
        const float lo=horizontal?std::min(r.start.x,r.end.x):std::min(r.start.y,r.end.y);
        const float hi=horizontal?std::max(r.start.x,r.end.x):std::max(r.start.y,r.end.y);
        if(along<lo || along>hi) continue;
        const float d=horizontal?std::abs(c.y-r.start.y):std::abs(c.x-r.start.x);
        if(d<best) {best=d;side=horizontal?(r.start.y<c.y?2:3):(r.start.x<c.x?0:1);}
    }
    return side;
}
inline float asphaltWidth(float corridor) { return std::max(1.2f,corridor-6); }
}
