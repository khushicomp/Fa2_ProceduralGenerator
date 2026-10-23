#include "VisualComposition.h"
#include "VisualAssets.h"
#include "VisualRoadStyle.h"
#include "StreetPresentation.h"
#include <glad/glad.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>
#include <cstdint>
#include <cstring>
namespace visual {
namespace {
using Rect=city::Boundary;
struct Paint { Rect bounds;std::array<float,3> color;bool round=false; };
std::vector<Paint> cachedPaint;
std::vector<BuildingArtwork> cachedBuildings;
std::array<float,3> ink{};
std::uint64_t cachedKey=0;bool ready=false;
void color(float r,float g,float b) { ink={r,g,b}; }
void colorv(const float* c) { color(c[0],c[1],c[2]); }
void fill(Rect r) { if(r.max.x>r.min.x && r.max.y>r.min.y) cachedPaint.push_back({r,ink,false}); }
void crown(float x,float y,float radius=6) {
    color(0.17f,0.28f,0.19f);
    cachedPaint.push_back({{{x-radius,y-radius},{x+radius,y+radius}},ink,true});
    color(0.29f,0.40f,0.25f);
    cachedPaint.push_back({{{x-radius+1,y-radius+2},{x+radius-2,y+radius-1}},ink,true});
}
void replay() {
    for(const auto& p:cachedPaint) {
        glColor3fv(p.color.data());const auto r=p.bounds;
        if(p.round) {
            const float x=(r.min.x+r.max.x)/2,y=(r.min.y+r.max.y)/2;
            glBegin(GL_TRIANGLE_FAN);glVertex2f(x,y);
            for(int i=0;i<=12;++i) {const float a=i*6.2831853f/12;glVertex2f(x+(r.max.x-r.min.x)/2*std::cos(a),y+(r.max.y-r.min.y)/2*std::sin(a));}
            glEnd();
        } else {
            glBegin(GL_QUADS);glVertex2f(r.min.x,r.min.y);glVertex2f(r.max.x,r.min.y);
            glVertex2f(r.max.x,r.max.y);glVertex2f(r.min.x,r.max.y);glEnd();
        }
    }
}
bool overlaps(Rect a,Rect b,float margin=0) {
    return a.max.x+margin>b.min.x && a.min.x-margin<b.max.x &&
           a.max.y+margin>b.min.y && a.min.y-margin<b.max.y;
}
bool inside(city::Point p,Rect r) {
    return p.x>=r.min.x && p.x<=r.max.x && p.y>=r.min.y && p.y<=r.max.y;
}
int category(const city::Building& b) {
    const float area=(b.max.x-b.min.x)*(b.max.y-b.min.y);
    return area<450?0:area<850?1:2;
}
city::Point center(Rect b) { return {(b.min.x+b.max.x)/2,(b.min.y+b.max.y)/2}; }
Rect around(city::Point p,float x,float y) {return {{p.x-x,p.y-y},{p.x+x,p.y+y}};}
bool nearFacility(Rect r,const city::Facilities& f) {
    for(const auto& h:f.hospitals) if(overlaps(r,around(h.position,16,16))) return true;
    for(const auto& e:f.emergencies) if(overlaps(r,around(e.position,16,16))) return true;
    return false;
}
// Decorative property paint only. All approved artwork records are complete before
// this pass, so an entrance cannot accidentally cross a later building's property.
void composeProperties(const std::vector<city::Block>& blocks,
                       const std::vector<city::Building>& buildings,
                       const std::vector<city::Road>& roads,const city::Facilities& facilities,
                       const std::vector<Rect>& envelopes,std::vector<Rect>& occupied,float pixelsPerUnit) {
    std::vector<Rect> obstacles,propertySurfaces;
    for(std::size_t i=0;i<buildings.size();++i) {
        obstacles.push_back({buildings[i].min,buildings[i].max});
        const auto& art=cachedBuildings[i];
        obstacles.push_back(art.texture.id?art.bounds:art.property);
        const float lawn[3]={0.28f,0.35f,0.25f},warm[3]={0.40f,0.40f,0.37f},cool[3]={0.38f,0.41f,0.40f};
        const float padding=std::min(3.0f,std::min(art.property.max.x-art.property.min.x,
                                                art.property.max.y-art.property.min.y)*0.05f);
        const auto reserved=envelopes[i];
        const Rect surface{{std::max(reserved.min.x,art.property.min.x-padding),std::max(reserved.min.y,art.property.min.y-padding)},
                           {std::min(reserved.max.x,art.property.max.x+padding),std::min(reserved.max.y,art.property.max.y+padding)}};
        propertySurfaces.push_back(surface);
        colorv(art.category==0?lawn:art.category==1?warm:cool);fill(surface);
    }
    const auto streets=streetProfiles(roads,obstacles); // Exact approved road dimensions.
    auto crossesStreet=[&](Rect r) {
        for(const auto& street:streets) for(std::size_t k=1;k<street.halfWidths.size();++k) {
            const float a=street.start+(k-1)*street.step,b=std::min(street.end,street.start+k*street.step);
            const float half=std::max(street.halfWidths[k-1],street.halfWidths[k]);
            const auto low=street.point(a,-half),high=street.point(b,half);
            if(overlaps(r,{{std::min(low.x,high.x),std::min(low.y,high.y)},
                           {std::max(low.x,high.x),std::max(low.y,high.y)}})) return true;
        }
        return false;
    };
    auto clear=[&](Rect r,std::size_t owner,Rect block) {
        if(r.max.x<=r.min.x || r.max.y<=r.min.y || !inside(r.min,block) || !inside(r.max,block) ||
           nearFacility(r,facilities) || crossesStreet(r)) return false;
        for(std::size_t j=0;j<buildings.size();++j) {
            if(j!=owner && (overlaps(r,propertySurfaces[j],0.5f) || overlaps(r,{buildings[j].min,buildings[j].max},1))) return false;
            const auto& art=cachedBuildings[j];
            if(overlaps(r,art.texture.id?art.bounds:art.property)) return false;
        }
        for(const auto& other:occupied) if(overlaps(r,other,0.5f)) return false;
        return true;
    };
    for(std::size_t i=0;i<buildings.size();++i) {
        const auto& art=cachedBuildings[i];const auto roof=art.texture.id?art.bounds:art.property;
        Rect block=art.property;
        for(const auto& b:blocks) if(inside(center(art.property),{b.min,b.max})) {block={b.min,b.max};break;}
        const int style=art.category;
        const float width=style==0?3.4f:style==3?7.0f:5.0f;
        float best=std::numeric_limits<float>::max(),bestAlong=0,bestEdge=0,bestEnd=0;
        int facing=-1;bool horizontal=false;Rect path{};
        // Prefer the nearest feasible perpendicular sidewalk. Alternative entrance
        // anchors on the same facade avoid a marker without moving the building.
        for(const auto& street:streets) for(float fraction:{0.5f,0.25f,0.75f}) {
            const bool h=street.horizontal;
            const float along=h?roof.min.x+(roof.max.x-roof.min.x)*fraction:roof.min.y+(roof.max.y-roof.min.y)*fraction;
            if(along-width/2<street.start || along+width/2>street.end) continue;
            const float c=h?center(roof).y:center(roof).x;
            const bool negative=street.coordinate<c;
            const float edge=h?(negative?roof.min.y:roof.max.y):(negative?roof.min.x:roof.max.x);
            float half=0;
            for(float q=along-width/2;q<=along+width/2+0.01f;q+=0.5f) half=std::max(half,street.halfWidth(q));
            const float end=street.coordinate+(negative?half+0.4f:-half-0.4f);
            if(negative?end>=edge:end<=edge) continue;
            const float distance=std::abs(edge-end);
            if(distance>=best || distance>(style==0?75.0f:110.0f)) continue;
            Rect candidate=h?Rect{{along-width/2,std::min(edge,end)},{along+width/2,std::max(edge,end)}}:
                             Rect{{std::min(edge,end),along-width/2},{std::max(edge,end),along+width/2}};
            if(!clear(candidate,i,block)) continue;
            best=distance;path=candidate;horizontal=h;bestAlong=along;bestEdge=edge;bestEnd=end;
            facing=h?(negative?2:3):(negative?0:1);
        }
        if(facing<0) continue;
        const float direction=bestEnd<bestEdge?-1.0f:1.0f;
        auto strip=[&](float along,float span,float near,float far) {
            return horizontal?Rect{{along-span/2,std::min(near,far)},{along+span/2,std::max(near,far)}}:
                              Rect{{std::min(near,far),along-span/2},{std::max(near,far),along+span/2}};
        };
        // A small doorstep/forecourt emphasizes the facade, not a large new parcel.
        const float faceWidth=horizontal?roof.max.x-roof.min.x:roof.max.y-roof.min.y;
        const float span=std::min(faceWidth*0.7f,style==0?12.0f:style==3?26.0f:22.0f);
        const float depth=std::min(best,style==0?4.0f:8.0f);
        const Rect frontage=strip(bestAlong,span,bestEdge,bestEdge+direction*depth);
        const bool hasFrontage=span>=width && clear(frontage,i,block);
        if(hasFrontage) {
            if(style==0) color(0.40f,0.43f,0.36f);else color(0.44f,0.46f,0.44f);
            fill(frontage);
        }
        color(style==3?0.62f:0.55f,style==3?0.63f:0.56f,style==3?0.59f:0.51f);fill(path);
        occupied.push_back(path);
        if(hasFrontage) occupied.push_back(frontage);
        // One narrow facade-edge planting strip where it fits. No trees or new
        // vegetation clusters: this only establishes a lawn/pavement transition.
        const float side=bestAlong+span/2+2;
        const Rect planting=strip(side,2.5f,bestEdge,bestEdge+direction*std::min(best,7.0f));
        if(clear(planting,i,block)) {color(0.25f,0.34f,0.23f);fill(planting);occupied.push_back(planting);}
        // Sparse, small parking/driveway choices use stable geometry, not a PRNG.
        const unsigned choice=static_cast<unsigned>(buildings[i].min.x*3+buildings[i].min.y*7);
        if(style==0 && choice%3==0 && best<32) {
            const Rect driveway=strip(bestAlong+span/2+7,5,bestEdge,bestEnd);
            if(clear(driveway,i,block)) {color(0.36f,0.38f,0.36f);fill(driveway);occupied.push_back(driveway);}
        } else if((style==1 || style==2) && choice%4==0 && best>=14) {
            const float parkingAlong=bestAlong+span/2+10;
            const Rect parking=strip(parkingAlong,16,bestEdge,bestEdge+direction*12);
            if(clear(parking,i,block)) {
                color(0.32f,0.35f,0.34f);fill(parking);occupied.push_back(parking);
                if(pixelsPerUnit>=1.2f) {
                    color(0.61f,0.61f,0.55f);
                    for(float offset:{-6.0f,0.0f,6.0f})
                        fill(strip(parkingAlong+offset,0.7f,bestEdge+direction*2,bestEdge+direction*10));
                }
            }
        }
    }
    // Give only a few genuinely large gaps a quiet shared lawn/courtyard surface.
    // Beveled corners and no outlines avoid a second grid of rectangular lots.
    int shared=0;
    for(const auto& b:blocks) {
        if(shared>=6) break;
        const Rect block{b.min,b.max};bool placed=false;
        for(float fy:{0.5f,0.3f,0.7f,0.15f,0.85f}) for(float fx:{0.5f,0.3f,0.7f,0.15f,0.85f}) {
            if(placed) continue;
            const auto c=city::Point{b.min.x+(b.max.x-b.min.x)*fx,b.min.y+(b.max.y-b.min.y)*fy};
            const float hw=std::min(24.0f,(b.max.x-b.min.x)*0.16f),hh=std::min(18.0f,(b.max.y-b.min.y)*0.16f);
            const Rect patch=around(c,hw,hh);
            if(hw<16 || hh<14 || !clear(patch,buildings.size(),block)) continue;
            color(0.25f,0.31f,0.24f);
            fill({{patch.min.x+4,patch.min.y},{patch.max.x-4,patch.max.y}});
            fill({{patch.min.x,patch.min.y+4},{patch.max.x,patch.max.y-4}});
            occupied.push_back(patch);placed=true;++shared;
        }
    }
}
}
const BuildingArtwork& buildingPresentation(std::size_t index) { return cachedBuildings.at(index); }
void composition(const std::vector<city::Block>& blocks,
                 const std::vector<city::Building>& buildings,
                 const std::vector<city::Road>& roads,const city::Facilities& facilities,float pixelsPerUnit) {
    // Key all read-only inputs; retain paint commands until geometry/facilities change.
    std::uint64_t key=1469598103934665603ull;
    auto number=[&](float x) {std::uint32_t bits;std::memcpy(&bits,&x,sizeof(bits));key=(key^bits)*1099511628211ull;};
    number(pixelsPerUnit>=1.2f?1.0f:0.0f);
    auto point=[&](city::Point p) {number(p.x);number(p.y);};
    number(static_cast<float>(blocks.size()));for(const auto& b:blocks) {point(b.min);point(b.max);}
    number(static_cast<float>(buildings.size()));for(const auto& b:buildings) {point(b.min);point(b.max);}
    number(static_cast<float>(roads.size()));for(const auto& r:roads) {point(r.start);point(r.end);}
    for(const auto& h:facilities.hospitals) point(h.position);
    for(const auto& e:facilities.emergencies) point(e.position);
    if(ready && cachedKey==key) {replay();return;}
    cachedPaint.clear();
    cachedBuildings.clear();cachedBuildings.reserve(buildings.size());
    // The largest building-category count gives each block a coherent palette.
    // Ties favor the smaller category. This is not zoning or generation state.
    std::vector<int> styles;styles.reserve(blocks.size());
    for(const auto& block:blocks) {
        const Rect bounds{block.min,block.max};
        std::array<int,3> counts{};
        for(const auto& b:buildings) if(inside(center({b.min,b.max}),bounds)) ++counts[category(b)];
        const int style=static_cast<int>(std::max_element(counts.begin(),counts.end())-counts.begin());
        styles.push_back(style);
        // A common terrain, rather than a separately framed rectangle per block.
        // Dominant categories influence parcel landscaping below, not block outlines.
    }
    std::vector<Rect> parcels;parcels.reserve(buildings.size());
    std::vector<Rect> approaches;approaches.reserve(buildings.size());
    for(const auto& b:buildings) {
        Rect parcel{b.min,b.max};const auto c=center(parcel);
        Rect bounds=parcel;
        for(std::size_t i=0;i<blocks.size();++i) {
            if(inside(c,{blocks[i].min,blocks[i].max})) {bounds={blocks[i].min,blocks[i].max};break;}
        }
        // Grow a shared artwork/property envelope. Each two-unit step is tested against
        // actual buildings, road corridors, earlier parcels, and marker clearances.
        for(int step=0;step<8;++step) for(int side=0;side<4;++side) {
            Rect candidate=parcel,strip=parcel;
            if(side==0) {candidate.min.x=std::max(bounds.min.x,parcel.min.x-2);strip.max.x=parcel.min.x;strip.min.x=candidate.min.x;}
            if(side==1) {candidate.max.x=std::min(bounds.max.x,parcel.max.x+2);strip.min.x=parcel.max.x;strip.max.x=candidate.max.x;}
            if(side==2) {candidate.min.y=std::max(bounds.min.y,parcel.min.y-2);strip.max.y=parcel.min.y;strip.min.y=candidate.min.y;}
            if(side==3) {candidate.max.y=std::min(bounds.max.y,parcel.max.y+2);strip.min.y=parcel.max.y;strip.max.y=candidate.max.y;}
            // A property's own facility belongs on its artwork. Protect other
            // markers without letting the hospital's own marker prevent growth.
            bool blocked=false;
            for(const auto& h:facilities.hospitals)
                if(!inside(h.position,{b.min,b.max}) && overlaps(strip,around(h.position,16,16))) blocked=true;
            for(const auto& e:facilities.emergencies)
                if(!inside(e.position,{b.min,b.max}) && overlaps(strip,around(e.position,16,16))) blocked=true;
            for(const auto& other:buildings) if(&other!=&b && overlaps(candidate,{other.min,other.max},2)) blocked=true;
            for(const auto& other:parcels) if(overlaps(candidate,other,1)) blocked=true;
            for(const auto& r:roads) {
                const float half=roadWidth(r,roads,buildings)/2+0.5f;
                if(overlaps(candidate,{{std::min(r.start.x,r.end.x)-half,std::min(r.start.y,r.end.y)-half},
                                        {std::max(r.start.x,r.end.x)+half,std::max(r.start.y,r.end.y)+half}})) blocked=true;
            }
            if(!blocked) parcel=candidate;
        }
        // Reserve the full envelope so later artwork cannot overlap it. The painted
        // property is compacted to the selected sprite's aspect, not enlarged alone.
        parcels.push_back(parcel);
        cachedBuildings.push_back(layoutBuilding(b,facilities,parcel));
    }
    composeProperties(blocks,buildings,roads,facilities,parcels,approaches,pixelsPerUnit);
    int clusters=0,parks=0;
    std::vector<Rect> planted;
    for(std::size_t index=0;index<blocks.size() && clusters<10;++index) {
        const auto& block=blocks[index];const Rect bounds{block.min,block.max};
        const int style=styles[index];
        auto clear=[&](Rect r) {
            if(!inside(r.min,bounds) || !inside(r.max,bounds) || nearFacility(r,facilities)) return false;
            for(const auto& b:buildings) if(overlaps(r,{b.min,b.max},3)) return false;
            for(const auto& road:roads) {
                const float half=roadWidth(road,roads,buildings)/2+1;
                if(overlaps(r,{{std::min(road.start.x,road.end.x)-half,std::min(road.start.y,road.end.y)-half},
                                {std::max(road.start.x,road.end.x)+half,std::max(road.start.y,road.end.y)+half}})) return false;
            }
            for(const auto& a:approaches) if(overlaps(r,a,2)) return false;
            for(const auto& p:planted) if(overlaps(r,p,4)) return false;
            return true;
        };
        // Search a small fixed set of positions; accept at most one group per block.
        // Ten groups globally, at most three larger park compositions.
        int inBlock=0;
        for(float fy:{0.25f,0.70f,0.48f}) for(float fx:{0.25f,0.70f,0.48f}) {
            if(clusters>=10 || inBlock>=(1)) continue;
            const city::Point c{block.min.x+(block.max.x-block.min.x)*fx,
                                block.min.y+(block.max.y-block.min.y)*fy};
            const Rect patch=around(c,12,12);
            if(!clear(patch)) continue;
            color(0.20f,0.29f,0.20f);fill(patch);
            color(0.24f,0.32f,0.23f);
            fill(around(c,10.5f,10.5f));
            const Rect park=around(c,24,18);
            if(parks<3 && clear(park)) {
                color(0.27f,0.34f,0.23f);fill(park);
                color(0.48f,0.47f,0.36f);fill(around(c,22,1.4f));
                fill({{c.x-1.4f,c.y-16},{c.x+1.4f,c.y+16}});
                crown(c.x-15,c.y-10,6);crown(c.x+15,c.y-10,6);
                crown(c.x-15,c.y+10,6);crown(c.x+15,c.y+10,6);
                planted.push_back(park);++parks;++clusters;++inBlock;continue;
            }
            crown(c.x-5,c.y-4);crown(c.x+5,c.y-3);
            if(style!=1) crown(c.x,c.y+5);
            planted.push_back(patch);++clusters;++inBlock;
        }
    }
    cachedKey=key;ready=true;replay();
}
}
