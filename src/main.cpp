#include "ApplicationInterface.h"
#include "Renderer.h"
#include "VisualAssets.h"
#include "SmokeValidation.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <charconv>
#include <exception>
#include <iostream>
#include <optional>
#include <string_view>

namespace {
void onKey(GLFWwindow* window,int key,int,int action,int) {
    if (action!=GLFW_PRESS || ImGui::GetIO().WantCaptureKeyboard) { return; }
    auto& app=*static_cast<city::CandidateApplication*>(glfwGetWindowUserPointer(window));
    try {
        switch(key) {
            case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window,GLFW_TRUE); break;
            case GLFW_KEY_R: app.generateCandidates(); break;
            case GLFW_KEY_O: app.optimizeSelectedCandidate(); break;
            case GLFW_KEY_B: app.toggleBeforeAfter(); break;
            case GLFW_KEY_P: app.selectIncident(1); break;
            case GLFW_KEY_G: app.setGraphVisible(!app.graphVisible); break;
            case GLFW_KEY_UP: app.adjustDensity(true,1); break;
            case GLFW_KEY_DOWN: app.adjustDensity(true,-1); break;
            case GLFW_KEY_RIGHT: app.adjustDensity(false,1); break;
            case GLFW_KEY_LEFT: app.adjustDensity(false,-1); break;
        }
    } catch(const std::exception& e) { app.status=e.what(); std::cerr<<e.what()<<'\n'; }
}
}
int main(int argc,char** argv) {
    bool smoke=false,demo=false,debug=false;
    std::optional<std::uint32_t> seed;
    for(int i=1;i<argc;++i) {
        const std::string_view arg(argv[i]);
        if(arg=="--smoke-test") { smoke=true; }
        else if(arg=="--demo") { demo=true; }
        else if(arg=="--debug") { debug=true; }
        else if(arg=="--seed") {
            if(++i==argc) { std::cerr<<"--seed requires an unsigned 32-bit integer.\n"; return 1; }
            const std::string_view value(argv[i]); std::uint32_t parsed=0;
            const auto conversion=std::from_chars(value.data(),value.data()+value.size(),parsed);
            if(conversion.ec!=std::errc{} || conversion.ptr!=value.data()+value.size()) {
                std::cerr<<"Invalid seed: "<<value<<'\n'; return 1;
            }
            seed=parsed;
        } else if(arg=="--help") {
            std::cout<<"Usage: ProceduralCityGenerator [--seed N] [--demo] [--debug] [--smoke-test]\n";
            return 0;
        } else { std::cerr<<"Unknown option: "<<arg<<'\n'; return 1; }
    }
    city::CandidateApplication app;
    app.debug=debug;
    if(demo) { app.setup.seed=42; app.setup.roadDensity=0.5f; app.setup.buildingDensity=0.5f; }
    if(seed) { app.setup.seed=*seed; }
    try { app.generateCandidates(); } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
    glfwSetErrorCallback([](int code,const char* text) { std::cerr<<"GLFW "<<code<<": "<<text<<'\n'; });
    if(!glfwInit()) { return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    if(smoke) { glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE); }
    GLFWwindow* window=glfwCreateWindow(1280,800,"Procedural City Generator",nullptr,nullptr);
    if(!window) { glfwTerminate(); return 1; }
    glfwSetWindowSizeLimits(window,1000,700,GLFW_DONT_CARE,GLFW_DONT_CARE);
    glfwMakeContextCurrent(window); glfwSwapInterval(smoke ? 0 : 1);
    if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) || !GLAD_GL_VERSION_2_1) {
        std::cerr<<"OpenGL 2.1 initialization failed.\n"; glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ui::ApplicationInterface interface;
    interface.initializeStyle();
    if(smoke) { ImGui::GetIO().ConfigInputTrickleEventQueue=false; }
    glfwSetWindowUserPointer(window,&app);
    glfwSetKeyCallback(window,onKey);
    if(!ImGui_ImplGlfw_InitForOpenGL(window,true)) {
        ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    if(!ImGui_ImplOpenGL2_Init()) {
        ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    std::cout<<"R Generate | O Optimize selected | B Before/After | P Next incident | G Graph | Arrows Pending densities | Escape Exit\n";
    visual::initialize();
    int result=0,frame=0;
    try {
        while(!glfwWindowShouldClose(window)) {
            interface.applyPendingAction();
            glfwPollEvents();
            if(glfwGetWindowAttrib(window,GLFW_ICONIFIED)) { glfwWaitEvents(); continue; }
            if(smoke) {
                // Retain earlier aspect ratios and add all Phase 7 presentation sizes.
                const int sizes[][2]={{1000,800},{1280,720},{1000,900},{1280,800},{1366,768},{1600,900},{1920,1080}};
                if(frame<7) { glfwSetWindowSize(window,sizes[frame][0],sizes[frame][1]); glfwPollEvents(); }
                if(frame==63) { glfwSetWindowSize(window,1000,700); glfwPollEvents(); }
                if(frame==64) { glfwSetWindowSize(window,1280,800); glfwPollEvents(); }
                if(frame==65) { glfwSetWindowSize(window,1366,768); glfwPollEvents(); }
                validation::keyboardStep(frame,window,app);
            }
            ImGui_ImplOpenGL2_NewFrame(); ImGui_ImplGlfw_NewFrame();
            if(smoke) { validation::mouseStep(frame,interface); }
            ImGui::NewFrame();
            const auto display=ImGui::GetIO().DisplaySize;
            const auto area=interface.draw(app,display.x,display.y);
            ImGui::Render();
            int fw=0,fh=0; glfwGetFramebufferSize(window,&fw,&fh);
            if(fw<=0 || fh<=0 || display.x<=0 || display.y<=0) { continue; }
            const float sx=fw/display.x,sy=fh/display.y;
            const int x=static_cast<int>(area.x*sx),y=static_cast<int>((display.y-area.y-area.height)*sy);
            const int width=static_cast<int>(area.width*sx),height=static_cast<int>(area.height*sy);
            glDisable(GL_SCISSOR_TEST);
            glClearColor(0.06f,0.08f,0.11f,1); glClear(GL_COLOR_BUFFER_BIT);
            glEnable(GL_SCISSOR_TEST); glScissor(x,y,width,height);
            renderer::resizeRegion(x,y,width,height);
            const auto& c=app.selected(); const auto& state=app.view();
            const std::vector<city::Road> noAddedRoads;
            renderer::draw(city::boundary,state.data.roads,state.data.buildings,
                           c.showAfter && c.optimization ? c.optimization->addedRoads : noAddedRoads,
                           state.data.blocks,state.facilities);
            if(app.graphVisible) { renderer::drawGraph(state.graph); }
            renderer::drawEmergency(state.graph,state.facilities,state.routes.empty() ? nullptr : &state.routes[c.selectedIncident]);
            if(smoke) { validation::checkFrame(frame,app,area,fw,fh,sx,sy); }
            glDisable(GL_SCISSOR_TEST);
            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
            if(smoke && glGetError()!=GL_NO_ERROR) { throw std::runtime_error("OpenGL GUI smoke error"); }
            glfwSwapBuffers(window);
            if(smoke && ++frame==validation::smokeFrames) { break; }
        }
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; result=1; }
    visual::shutdown();
    ImGui_ImplOpenGL2_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwDestroyWindow(window); glfwTerminate();
    if(smoke && !result) { std::cout<<"Smoke test passed: keyboard, mouse widgets, snapshots, rendering and resizing.\n"; }
    return result;
}
