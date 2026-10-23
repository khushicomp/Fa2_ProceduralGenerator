#include "TextureManager.h"
#include "VisualAssets.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
    if(!glfwInit()) return 1;
    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    GLFWwindow* window=glfwCreateWindow(100,100,"Asset validation",nullptr,nullptr);
    if(!window) {glfwTerminate();return 1;}
    glfwMakeContextCurrent(window);
    int result=0;
    try {
        check(gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))!=0,"GLAD");
        visual::TextureManager cache;
        const auto root=std::filesystem::path(VISUAL_ASSET_DIR);
        const auto& t=cache.load(root/"buildings/residential/01.png");
        check(t.id && t.width>0 && t.height>0,"PNG failed to load");
        check(&t==&cache.load(root/"buildings/residential/01.png"),"Texture was not cached");
        std::vector<unsigned char> pixels(t.width*t.height*4);
        glBindTexture(GL_TEXTURE_2D,t.id);glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        check(pixels[3]==0,"Transparent padding lost");
        // The supplied artwork has slightly translucent interior pixels (252 alpha).
        check(pixels[(t.height/2*t.width+t.width/2)*4+3]>=240,"Roof unexpectedly transparent");
        check(!cache.load(root/"missing.png").id,"Missing asset did not fall back");
        check(!cache.load(root/"README.md").id,"Non-PNG asset did not fall back");
        const auto id=t.id;cache.clear();check(!glIsTexture(id),"GPU texture not released");
        visual::initialize();
        glViewport(0,0,100,100);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,100,0,100,-1,1);
        glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        const city::Building b{{20,30},{70,60}};
        auto capture=[&] {
            glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
            check(visual::building(b,{}),"Building asset fallback despite available textures");
            std::vector<unsigned char> image(100*100*4);
            glReadPixels(0,0,100,100,GL_RGBA,GL_UNSIGNED_BYTE,image.data());return image;
        };
        const auto first=capture();check(first==capture(),"Asset appearance changed across frames");
        int colored=0;
        for(int y=0;y<100;++y) for(int x=0;x<100;++x) {
            const int offset=(y*100+x)*4;
            if(first[offset] || first[offset+1] || first[offset+2]) {
                ++colored;check(x>=20 && x<70 && y>=30 && y<60,"Sprite escaped building footprint");
            }
        }
        check(colored>100,"Sprite invisible");
        visual::setDetailed(false);check(!visual::building(b,{}),"Simplified toggle failed");
        visual::setDetailed(true);check(first==capture(),"Toggle changed deterministic sprite");
        check(glGetError()==GL_NO_ERROR,"OpenGL asset error");
        std::cout<<"Texture cache, alpha, fallback, release, contain bounds, deterministic drawing and toggle passed.\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';result=1;}
    visual::shutdown();glfwDestroyWindow(window);glfwTerminate();return result;
}
