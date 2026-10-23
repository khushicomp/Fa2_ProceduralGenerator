#include "Renderer.h"
#include "VisualAssets.h"
#include "VisualComposition.h"
#include "VisualRoadStyle.h"
#include "StreetPresentation.h"

#include <glad/glad.h>
#include <algorithm>
#include <cmath>

namespace renderer {
namespace {
// Only rendering scale is cached; simulation geometry is always read-only.
float pixelsPerUnit = 1.0f;
float markerScale() { return std::max(1.0f, 0.85f / pixelsPerUnit); }
void box(float left, float bottom, float right, float top) {
    glBegin(GL_QUADS);
    glVertex2f(left,bottom); glVertex2f(right,bottom);
    glVertex2f(right,top); glVertex2f(left,top);
    glEnd();
}
bool sameRoad(const city::Road& a, const city::Road& b) {
    auto same = [](city::Point p, city::Point q) { return p.x==q.x && p.y==q.y; };
    return (same(a.start,b.start) && same(a.end,b.end)) ||
           (same(a.start,b.end) && same(a.end,b.start));
}
// Portable world-space stroke, independent of driver wide-line support.
void stroke(city::Point a, city::Point b, float width) {
    const float length = std::hypot(b.x-a.x,b.y-a.y);
    if (length == 0) { return; }
    const float dx = -(b.y-a.y)*width/(2*length), dy = (b.x-a.x)*width/(2*length);
    glBegin(GL_QUADS);
    glVertex2f(a.x+dx,a.y+dy); glVertex2f(b.x+dx,b.y+dy);
    glVertex2f(b.x-dx,b.y-dy); glVertex2f(a.x-dx,a.y-dy);
    glEnd();
}
// A union of tapered quads per layer creates continuous sidewalks/curbs and
// merged asphalt at crossings, with no border drawn through an intersection.
void streetStrip(const visual::StreetProfile& p,float ratio) {
    glBegin(GL_QUADS);
    for(std::size_t i=1;i<p.halfWidths.size();++i) {
        const float a=p.start+(i-1)*p.step,b=std::min(p.end,p.start+i*p.step);
        const auto p0=p.point(a,-p.halfWidths[i-1]*ratio),p1=p.point(b,-p.halfWidths[i]*ratio);
        const auto p2=p.point(b,p.halfWidths[i]*ratio),p3=p.point(a,p.halfWidths[i-1]*ratio);
        glVertex2f(p0.x,p0.y);glVertex2f(p1.x,p1.y);glVertex2f(p2.x,p2.y);glVertex2f(p3.x,p3.y);
    }
    glEnd();
}
void drawStreets(const std::vector<city::Road>& roads,const std::vector<city::Building>& buildings,
                 const std::vector<city::Road>& addedRoads) {
    if(!visual::detailed()) {
        for(const auto& road:roads) {
            const bool added=std::any_of(addedRoads.begin(),addedRoads.end(),[&](const auto& r){return sameRoad(road,r);});
            if(added) glColor3f(0.26f,0.61f,0.58f);else glColor3f(0.40f,0.44f,0.43f);
            stroke(road.start,road.end,2);
            glColor3f(0.095f,0.12f,0.14f);stroke(road.start,road.end,1.2f);
        }
        return;
    }
    // Preserve Stage 1 exactly. Streets may use empty property margins, but must
    // clear both actual artwork/fallback bounds and every simulation footprint.
    std::vector<city::Boundary> obstacles;obstacles.reserve(buildings.size()*2);
    for(std::size_t i=0;i<buildings.size();++i) {
        obstacles.push_back({buildings[i].min,buildings[i].max});
        const auto& art=visual::buildingPresentation(i);
        obstacles.push_back(art.texture.id?art.bounds:art.property);
    }
    const auto streets=visual::streetProfiles(roads,obstacles);
    const auto junctions=visual::streetJunctions(streets);
    glColor3f(0.53f,0.54f,0.51f);
    for(const auto& p:streets) streetStrip(p,1);
    for(std::size_t i=0;i<streets.size();++i) {
        const bool added=std::any_of(addedRoads.begin(),addedRoads.end(),[&](const auto& r){return sameRoad(roads[i],r);});
        if(added) glColor3f(0.23f,0.65f,0.61f);else glColor3f(0.40f,0.42f,0.41f);
        streetStrip(streets[i],visual::streetCurbRatio);
    }
    glColor3f(0.17f,0.19f,0.20f);
    for(const auto& p:streets) streetStrip(p,visual::streetAsphaltRatio);
    for(std::size_t i=0;i<streets.size();++i) {
        const auto& p=streets[i];
        auto junctionDistance=[&](float along,float extra) {
            for(const auto& j:junctions) if(j.first==i || j.second==i) {
                const auto& other=streets[j.first==i?j.second:j.first];
                const float at=p.horizontal?j.position.x:j.position.y;
                const float across=p.horizontal?j.position.y:j.position.x;
                if(std::abs(along-at)<other.halfWidth(across)+extra) return true;
            }
            return false;
        };
        glColor3f(0.76f,0.75f,0.65f);
        for(float at=p.start+10;at+7<p.end-8;at+=18) {
            // Keep the established approach gap; ordinary junctions have no crosswalk bars.
            if(junctionDistance(at+3.5f,12) || p.halfWidth(at)*visual::streetAsphaltRatio<4) continue;
            stroke(p.point(at),p.point(at+7),1.05f);
        }

    }
}

}

void resize(int framebufferWidth, int framebufferHeight) {
    resizeRegion(0,0,framebufferWidth,framebufferHeight);
}
void resizeRegion(int x, int y, int framebufferWidth, int framebufferHeight) {
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return; // A minimized window can have a zero-sized framebuffer.
    }

    // Letterbox the fixed world so resizing never stretches the city.
    const float scale = std::min(framebufferWidth / city::worldWidth,
                                 framebufferHeight / city::worldHeight);

    const int width = std::max(1, static_cast<int>(city::worldWidth * scale));
    const int height = std::max(1, static_cast<int>(city::worldHeight * scale));
    glViewport(x + (framebufferWidth - width) / 2,
               y + (framebufferHeight - height) / 2, width, height);

    // Cartesian 2D coordinates: bottom-left (0,0), +x right, +y up.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Fit the city boundary with a small frame, preserving the viewport aspect.
    const float viewWidth=(city::boundary.max.x-city::boundary.min.x)+36.0f;
    const float viewHeight=viewWidth*city::worldHeight/city::worldWidth;
    const float cx=(city::boundary.min.x+city::boundary.max.x)/2;
    const float cy=(city::boundary.min.y+city::boundary.max.y)/2;
    pixelsPerUnit=width/viewWidth;
    glOrtho(cx-viewWidth/2,cx+viewWidth/2,cy-viewHeight/2,cy+viewHeight/2,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void draw(const city::Boundary& boundary, const std::vector<city::Road>& roads,
          const std::vector<city::Building>& buildings,
          const std::vector<city::Road>& addedRoads,
          const std::vector<city::Block>& blocks,
          const city::Facilities& facilities) {
    glClearColor(0.06f, 0.08f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Quiet ground separates the generated area from the application background.
    glColor3f(0.22f,0.27f,0.23f);
    box(boundary.min.x,boundary.min.y,boundary.max.x,boundary.max.y);
    if (visual::detailed()) { visual::composition(blocks,buildings,roads,facilities,pixelsPerUnit); }

    for (std::size_t index=0;index<buildings.size();++index) {
        const auto& building=buildings[index];
        // Artwork is the normal detailed representation for every category.
        // Only missing/unusable assets enter the existing procedural fallback.
        if(visual::detailed() && visual::drawBuilding(visual::buildingPresentation(index))) continue;
        const auto fallback=visual::detailed()?visual::buildingPresentation(index).property:
                                              city::Boundary{building.min,building.max};
        const auto a=fallback.min,b=fallback.max;
        const float w=b.x-a.x,h=b.y-a.y;
        if(!visual::detailed()) {
            glColor3f(0.30f,0.37f,0.41f);box(a.x,a.y,b.x,b.y);continue;
        }
        const int category=w*h<450?0:w*h<850?1:2;
        const int variant=static_cast<int>(a.x*3+a.y*7)%3;
        const float projected=std::min(w,h)*pixelsPerUnit;
        bool hospital=false;
        for(const auto& facility:facilities.hospitals) {
            const auto p=facility.position;
            if(p.x>=a.x && p.x<=b.x && p.y>=a.y && p.y<=b.y) hospital=true;
        }
        // Architectural forms occupy 92% of the footprint area, with no stretching.
        const float left=a.x+w*0.02f,right=b.x-w*0.02f;
        const float bottom=a.y+h*0.02f,top=b.y-h*0.02f;
        glColor3f(0.12f,0.17f,0.19f);box(left,bottom,right,top);
        const float outline=std::min(1.0f,0.7f/pixelsPerUnit);
        const float colors[3][3]={{0.64f,0.58f,0.47f},{0.60f,0.52f,0.41f},{0.44f,0.54f,0.60f}};
        if(hospital) glColor3f(0.72f,0.83f,0.80f); else glColor3fv(colors[category]);
        box(left+outline,bottom+outline,right-outline,top-outline);
        if(category==0 && !hospital) {
            const float roofs[3][3]={{0.53f,0.34f,0.27f},{0.36f,0.42f,0.46f},{0.48f,0.39f,0.30f}};
            const float l=left+1.3f,r=right-1.3f,bt=bottom+1.3f,t=top-1.3f;
            const float cx=(l+r)/2,cy=(bt+t)/2;
            const auto& color=roofs[variant];
            // Broad roof planes are the house silhouette, not micro-detail.
            glBegin(GL_TRIANGLES);
            glColor3f(color[0]+0.13f,color[1]+0.11f,color[2]+0.09f);
            glVertex2f(l,t);glVertex2f(r,t);glVertex2f(cx,cy);
            glColor3f(color[0]-0.06f,color[1]-0.06f,color[2]-0.06f);
            glVertex2f(l,bt);glVertex2f(r,bt);glVertex2f(cx,cy);
            glColor3fv(color);glVertex2f(l,bt);glVertex2f(l,t);glVertex2f(cx,cy);
            glColor3f(color[0]+0.05f,color[1]+0.04f,color[2]+0.03f);
            glVertex2f(r,bt);glVertex2f(r,t);glVertex2f(cx,cy);glEnd();
        } else if(projected>=16) {
            if(hospital) glColor3f(0.22f,0.48f,0.46f);
            else glColor3f(colors[category][0]-0.075f,colors[category][1]-0.075f,colors[category][2]-0.075f);
            box(left+2.5f,bottom+2.5f,right-2.5f,top-2.5f);
            if(category==1) {glColor3f(0.69f,0.56f,0.41f);box(left+2,bottom+1,right-2,bottom+3);}
            if(projected>=36) {
                glColor3f(0.56f,0.63f,0.65f);
                for(int i=0;i<(category==2?3:1);++i) {
                    const float x=left+(right-left)*(i+1)/(category==2?4.0f:2.0f);
                    box(x-2.5f,(bottom+top)/2-2.5f,x+2.5f,(bottom+top)/2+2.5f);
                }
            }
        }
        if(projected>=20) {
            const int facing=visual::roadFacing(building,roads);
            glColor3f(0.74f,0.72f,0.62f);
            const float cx=(left+right)/2,cy=(bottom+top)/2;
            if(facing==0) box(left,cy-3,left+2,cy+3);
            if(facing==1) box(right-2,cy-3,right,cy+3);
            if(facing==2) box(cx-3,bottom,cx+3,bottom+2);
            if(facing==3) box(cx-3,top-2,cx+3,top);
        }

    }

    drawStreets(roads,buildings,addedRoads);
    glColor3f(0.39f,0.46f,0.46f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(boundary.min.x,boundary.min.y); glVertex2f(boundary.max.x,boundary.min.y);
    glVertex2f(boundary.max.x,boundary.max.y); glVertex2f(boundary.min.x,boundary.max.y);
    glEnd();
}

void drawGraph(const city::RoadGraph& graph) {
    // Dim the map before analytical overlays; routes and facilities are drawn later.
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.035f,0.055f,0.07f,0.28f);
    box(city::boundary.min.x-9,city::boundary.min.y-9,city::boundary.max.x+9,city::boundary.max.y+9);
    glPopAttrib();
    glColor3f(0.60f, 0.73f, 0.84f);
    glBegin(GL_LINES);
    for (const auto& edge : graph.edges) {
        const auto a = graph.nodes[edge.from].position;
        const auto b = graph.nodes[edge.to].position;
        glVertex2f(a.x, a.y);
        glVertex2f(b.x, b.y);
    }
    glEnd();
    glColor3f(0.85f, 0.91f, 1.0f);
    glPointSize(std::clamp(4.0f*pixelsPerUnit,3.0f,5.0f));
    glBegin(GL_POINTS);
    for (const auto& node : graph.nodes) { glVertex2f(node.position.x, node.position.y); }
    glEnd();
    glPointSize(1.0f);
}

void drawEmergency(const city::RoadGraph& graph, const city::Facilities& facilities,
                   const city::EmergencyRouteResult* selected) {
    auto rectangle = [](float x, float y, float halfWidth, float halfHeight) {
        halfWidth *= markerScale(); halfHeight *= markerScale();
        glBegin(GL_QUADS);
        glVertex2f(x - halfWidth, y - halfHeight); glVertex2f(x + halfWidth, y - halfHeight);
        glVertex2f(x + halfWidth, y + halfHeight); glVertex2f(x - halfWidth, y + halfHeight);
        glEnd();
    };
    // Subtle access connectors are not road edges and do not contribute distance.
    glColor3f(0.37f, 0.42f, 0.42f);
    glBegin(GL_LINES);
    auto connector = [&](city::Point position, int node) {
        glVertex2f(position.x, position.y);
        glVertex2f(graph.nodes[node].position.x, graph.nodes[node].position.y);
    };
    if (selected) {
        for (const auto& e : facilities.emergencies) {
            if (e.id == selected->emergencyId) { connector(e.position, e.associatedGraphNodeId); }
        }
        for (const auto& h : facilities.hospitals) {
            if (h.id == selected->hospitalId) { connector(h.position, h.associatedGraphNodeId); }
        }
    }
    glEnd();
    // Thick quads avoid implementation-dependent wide OpenGL line support.
    glColor3f(0.98f, 0.78f, 0.22f);
    if (selected && selected->reachable) {
        const auto& path = selected->pathNodeIds;
        glColor3f(0.26f,0.22f,0.10f);
        for(std::size_t i=1;i<path.size();++i) stroke(graph.nodes[path[i-1]].position,graph.nodes[path[i]].position,9);
        glColor3f(1.0f,0.80f,0.20f);
        for (std::size_t i = 1; i < path.size(); ++i) {
            const city::Point a = graph.nodes[path[i - 1]].position, b = graph.nodes[path[i]].position;
            stroke(a,b,7.0f);
        }
        for (int id : path) { const city::Point p = graph.nodes[id].position; rectangle(p.x, p.y, 3.5f / markerScale(), 3.5f / markerScale()); }
    }
    for (const auto& h : facilities.hospitals) {
        if (selected && h.id == selected->hospitalId) {
            glColor3f(0.98f, 0.78f, 0.22f); rectangle(h.position.x, h.position.y, 12, 12);
        }
        glColor3f(0.90f,0.96f,0.93f); rectangle(h.position.x,h.position.y,9.5f,9.5f);
        glColor3f(0.06f, 0.51f, 0.36f); rectangle(h.position.x, h.position.y, 8, 8);
        glColor3f(1, 1, 1);
        rectangle(h.position.x, h.position.y, 5, 1.5f);
        rectangle(h.position.x, h.position.y, 1.5f, 5);
    }
    for (const auto& e : facilities.emergencies) {
        auto diamond=[&](float radius) {
            radius*=markerScale();glBegin(GL_QUADS);
            glVertex2f(e.position.x,e.position.y+radius);glVertex2f(e.position.x+radius,e.position.y);
            glVertex2f(e.position.x,e.position.y-radius);glVertex2f(e.position.x-radius,e.position.y);glEnd();
        };
        if(selected && e.id==selected->emergencyId) {glColor3f(1.0f,0.80f,0.20f);diamond(13);}
        glColor3f(0.98f,0.94f,0.88f);diamond(10);
        glColor3f(0.95f,0.27f,0.21f);diamond(7.8f);
    }
}

} // namespace renderer
