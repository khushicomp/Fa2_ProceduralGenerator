#pragma once
#include "City.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace visual {
// Render-only street profiles. The approved building layout and simulation road
// centerlines are inputs, never modified. Width varies only where clearance varies.
struct StreetProfile {
    bool horizontal;
    float coordinate, start, end, step;
    std::vector<float> halfWidths;
    float halfWidth(float along) const {
        const float sample=std::clamp((along-start)/step,0.0f,float(halfWidths.size()-1));
        const auto i=std::min(static_cast<std::size_t>(sample),halfWidths.size()-2);
        return halfWidths[i]+(halfWidths[i+1]-halfWidths[i])*(sample-float(i));
    }
    city::Point point(float along,float offset=0) const {
        return horizontal?city::Point{along,coordinate+offset}:city::Point{coordinate+offset,along};
    }
};
struct StreetJunction { std::size_t first,second; city::Point position; };
inline std::vector<StreetProfile> streetProfiles(const std::vector<city::Road>& roads,
                                                const std::vector<city::Boundary>& obstacles) {
    std::vector<StreetProfile> result;
    for(const auto& road:roads) {
        const bool h=road.start.y==road.end.y;
        const float start=h?std::min(road.start.x,road.end.x):std::min(road.start.y,road.end.y);
        const float end=h?std::max(road.start.x,road.end.x):std::max(road.start.y,road.end.y);
        const int segments=std::max(1,int(std::ceil((end-start)/6.0f)));
        StreetProfile p{h,h?road.start.y:road.start.x,start,end,std::max(0.001f,(end-start)/segments),{}};
        float maximum=16; // Up to 32 world units: sidewalk, curb, asphalt, curb, sidewalk.
        for(const auto& other:roads) if(h==(other.start.y==other.end.y)) {
            const float gap=std::abs(p.coordinate-(h?other.start.y:other.start.x));
            if(gap>0.01f) maximum=std::min(maximum,gap*0.325f);
        }
        for(int i=0;i<=segments;++i) {
            const float along=start+i*p.step;
            float half=maximum;
            for(const auto& b:obstacles) {
                const float lo=h?b.min.x:b.min.y,hi=h?b.max.x:b.max.y;
                // Include both neighboring sample intervals. Interpolation then
                // cannot cut through an obstacle between the sample positions.
                if(hi<along-p.step || lo>along+p.step) continue;
                const float low=h?b.min.y:b.min.x,high=h?b.max.y:b.max.x;
                const float gap=p.coordinate<low?low-p.coordinate:p.coordinate>high?p.coordinate-high:0;
                half=std::min(half,std::max(0.0f,gap-0.75f));
            }
            p.halfWidths.push_back(half);
        }
        // Keep each street block straight-sided. Using every small clearance gap
        // as a widening opportunity would create visibly wavy sidewalks.
        std::vector<float> cuts{start,end};
        for(const auto& other:roads) if(h!=(other.start.y==other.end.y)) {
            const float at=h?other.start.x:other.start.y;
            const float low=h?std::min(other.start.y,other.end.y):std::min(other.start.x,other.end.x);
            const float high=h?std::max(other.start.y,other.end.y):std::max(other.start.x,other.end.x);
            if(at>start && at<end && p.coordinate>=low && p.coordinate<=high) cuts.push_back(at);
        }
        std::sort(cuts.begin(),cuts.end());
        const auto limits=p.halfWidths;
        for(std::size_t c=1;c<cuts.size();++c) {
            const int first=std::max(0,int(std::floor((cuts[c-1]-start)/p.step)));
            const int last=std::min(segments,int(std::ceil((cuts[c]-start)/p.step)));
            float half=maximum;
            for(int i=first;i<=last;++i) half=std::min(half,limits[i]);
            for(int i=first;i<=last;++i) p.halfWidths[i]=std::min(p.halfWidths[i],half);
        }
        // Only reduce widths to form gentle tapers; never smooth outward through
        // a protected building. An isolated obstruction no longer narrows a road globally.
        const float slope=p.step*0.16f;
        for(std::size_t i=1;i<p.halfWidths.size();++i)
            p.halfWidths[i]=std::min(p.halfWidths[i],p.halfWidths[i-1]+slope);
        for(std::size_t i=p.halfWidths.size()-1;i>0;--i)
            p.halfWidths[i-1]=std::min(p.halfWidths[i-1],p.halfWidths[i]+slope);
        result.push_back(std::move(p));
    }
    return result;
}
inline std::vector<StreetJunction> streetJunctions(const std::vector<StreetProfile>& streets) {
    std::vector<StreetJunction> result;
    for(std::size_t i=0;i<streets.size();++i) for(std::size_t j=i+1;j<streets.size();++j) {
        if(streets[i].horizontal==streets[j].horizontal) continue;
        const auto& h=streets[i].horizontal?streets[i]:streets[j];
        const auto& v=streets[i].horizontal?streets[j]:streets[i];
        if(v.coordinate>=h.start && v.coordinate<=h.end && h.coordinate>=v.start && h.coordinate<=v.end)
            result.push_back({i,j,{v.coordinate,h.coordinate}});
    }
    return result;
}
inline constexpr float streetAsphaltRatio=0.73f;
inline constexpr float streetCurbRatio=0.79f;
} // namespace visual
