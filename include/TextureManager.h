#pragma once
#include <filesystem>
#include <map>
#include <string>

namespace visual {
struct Texture { unsigned int id=0; int width=0,height=0;
    int left=0,top=0,right=0,bottom=0; // Exclusive nonzero-alpha bounds.
};
// GPU resources must be cleared while the owning OpenGL context is current.
class TextureManager {
public:
    ~TextureManager();
    TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    const Texture& load(const std::filesystem::path& path);
    void clear();
private:
    std::map<std::filesystem::path,Texture> textures_;
};
}
