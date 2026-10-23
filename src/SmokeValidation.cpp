#include "SmokeValidation.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace validation {
namespace {
void check(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
bool sameRoads(const std::vector<city::Road>& a, const std::vector<city::Road>& b) {
    if (a.size()!=b.size()) { return false; }
    for (std::size_t i=0;i<a.size();++i) {
        if (a[i].start.x!=b[i].start.x || a[i].start.y!=b[i].start.y || a[i].end.x!=b[i].end.x || a[i].end.y!=b[i].end.y) { return false; }
    }
    return true;
}
}
void keyboardStep(int frame, GLFWwindow* window, city::CandidateApplication& app) {
    const auto callback = glfwSetKeyCallback(window,nullptr); glfwSetKeyCallback(window,callback);
    auto press = [&](int key, int action = GLFW_PRESS) {
        callback(window,key,0,action,0);
        if (action == GLFW_PRESS) { callback(window,key,0,GLFW_RELEASE,0); }
    };
    // Testing shortcuts uses the registered backend callback, which chains the application callback.
    if (frame <= 2) { ImGui::GetIO().WantCaptureKeyboard = false; }
    if (frame==0) {
        press(GLFW_KEY_B); check(!app.selected().optimization && !app.selected().showAfter,"B before optimization");
    }
    if (frame==1) {
        const auto original = app.selected().original;
        const auto seed = app.selected().parameters.seed;
        press(GLFW_KEY_O);
        const auto& c = app.selected();
        check(c.optimization && c.showAfter && c.parameters.seed==seed,"O snapshot failed");
        check(sameRoads(original.data.roads,c.original.data.roads) &&
              original.data.buildings.size()==c.original.data.buildings.size(),"O changed original geometry");
        check(c.optimization->optimized.score.finalScore>=original.score.finalScore,"O worsened score");
        press(GLFW_KEY_G); press(GLFW_KEY_G,GLFW_REPEAT);
        check(app.graphVisible,"G on/repeat failed");
        for (std::size_t i=0;i<=app.view().routes.size() && !app.view().routes.empty();++i) {
            press(GLFW_KEY_P); press(GLFW_KEY_P,GLFW_REPEAT);
            check(app.selected().selectedIncident==(i+1)%app.view().routes.size(),"P cycling/repeat failed");
        }
        // Restore route 0 for mouse tests, with the same shared action.
        if (!app.view().routes.empty()) { app.selectIncident(-static_cast<int>(app.selected().selectedIncident)); }
    }
    if (frame==2) {
        press(GLFW_KEY_B);
        check(!app.selected().showAfter && &app.view()==&app.selected().original,"B original snapshot failed");
        press(GLFW_KEY_B);
        check(app.selected().showAfter && &app.view()==&app.selected().optimization->optimized,"B optimized snapshot failed");
        press(GLFW_KEY_B); press(GLFW_KEY_G); press(GLFW_KEY_G,GLFW_REPEAT);
        check(!app.graphVisible,"G off/repeat failed");
    }
    if (frame==46) {
        const auto index=app.selected().selectedIncident;
        check(ImGui::GetIO().WantCaptureKeyboard,"Seed field did not capture keyboard");
        press(GLFW_KEY_P);
        check(app.selected().selectedIncident==index,"Shortcut fired while editing seed");
    }
}
void mouseStep(int frame, const ui::ApplicationInterface& interface) {
    // Actual Dear ImGui mouse events, not direct calls to application actions.
    // Hover before pressing: tab widgets intentionally require a prior hover
    // frame for overlapping hit areas, as with real mouse movement.
    const char* target = nullptr;
    switch (frame) {
        case 7: target="tab.analysis"; break;
        case 10: target="graph"; break;
        case 13: target="incident.next"; break;
        case 16: target="incident.previous"; break;
        case 19: target="tab.optimize"; break;
        case 22: target="after"; break;
        case 25: target="before"; break;
        case 28: target="tab.setup"; break;
        case 31: target="candidate.1"; break;
        case 34: target="tab.optimize"; break;
        case 37: target="optimize"; break;
        case 40: target="tab.setup"; break;
        case 43: target="seed"; break;
        case 50: target="density.road"; break;
        case 53: target="density.building"; break;
        case 56: target="count"; break;
        case 59: target="generate"; break;
    }
    auto& io = ImGui::GetIO();
    io.AddFocusEvent(true);
    if (target) {
        const auto it=interface.targets.find(target);
        if (it==interface.targets.end()) { throw std::runtime_error(std::string("UI smoke target missing: ")+target); }
        const auto& r=it->second;
        const float fraction=frame==50 ? 0.70f : (frame==53 ? 0.30f : (frame==56 ? 0.75f : 0.50f));
        io.AddMousePosEvent(r.x+r.width*fraction,r.y+r.height/2);
    }
    const bool press = (frame>=7 && frame<=45 && (frame-7)%3==1) ||
                       (frame>=50 && frame<=61 && (frame-50)%3==1);
    io.AddMouseButtonEvent(0,press);
    if (frame==46) { io.AddKeyEvent(ImGuiMod_Ctrl,true); io.AddKeyEvent(ImGuiKey_A,true); }
    if (frame==47) {
        io.AddKeyEvent(ImGuiKey_A,false); io.AddKeyEvent(ImGuiMod_Ctrl,false);
        io.AddInputCharactersUTF8("123");
    }
    if (frame==48) { io.AddKeyEvent(ImGuiKey_Enter,true); }
    if (frame==49) { io.AddKeyEvent(ImGuiKey_Enter,false); }
}
void checkFrame(int frame, const city::CandidateApplication& app, ui::Rect area,
                int fw, int fh, float sx, float sy) {
    const int x=static_cast<int>(area.x*sx), y=static_cast<int>((ImGui::GetIO().DisplaySize.y-area.y-area.height)*sy);
    const int width=static_cast<int>(area.width*sx), height=static_cast<int>(area.height*sy);
    const float scale=std::min(width/city::worldWidth,height/city::worldHeight);
    GLint v[4]; glGetIntegerv(GL_VIEWPORT,v);
    const int vw=static_cast<int>(city::worldWidth*scale), vh=static_cast<int>(city::worldHeight*scale);
    check(v[0]==x+(width-vw)/2 && v[1]==y+(height-vh)/2 && v[2]==vw && v[3]==vh,"Sidebar viewport/letterbox failed");
    check(v[0]>=0 && v[1]>=0 && v[0]+v[2]<=fw && v[1]+v[3]<=fh,"Viewport clipped");
    if (frame==13) { check(app.graphVisible,"Mouse graph checkbox did not update shared state"); }
    if (frame==16 && !app.view().routes.empty()) { check(app.selected().selectedIncident==1%app.view().routes.size(),"Mouse Next failed"); }
    if (frame==19) { check(app.selected().selectedIncident==0,"Mouse Previous failed"); }
    if (frame==25) { check(app.selected().showAfter,"Mouse After failed"); }
    if (frame==28) { check(!app.selected().showAfter,"Mouse Before failed"); }
    if (frame==34) {
        check(app.selectedIndex()==1 && &app.view()==&app.candidates()[1].original,"Mouse candidate selection failed");
    }
    if (frame==40) {
        check(app.selectedIndex()==1 && app.selected().optimization && app.selected().showAfter,"Mouse optimize failed");
        check(!app.candidates()[2].optimization,"Mouse optimized unrelated candidate");
    }
    if (frame==49) { check(app.setup.seed==123,"Seed text input failed"); }
    if (frame==52) { check(app.setup.roadDensity>0.6f && app.selected().parameters.roadDensity==0.5f,"Road slider regenerated early"); }
    if (frame==55) { check(app.setup.buildingDensity<0.5f && app.selected().parameters.buildingDensity==0.5f,"Building slider regenerated early"); }
    if (frame==58) { check(app.candidateCount==4 && app.candidates().size()==3,"Candidate count control failed"); }
    if (frame==62) {
        check(app.candidates().size()==4 && app.selectedIndex()==0 && app.selected().parameters.seed==123 &&
              app.selected().parameters.roadDensity==app.setup.roadDensity &&
              app.selected().parameters.buildingDensity==app.setup.buildingDensity && !app.selected().optimization,
              "Mouse Generate Candidates failed");
    }
}
} // namespace validation
