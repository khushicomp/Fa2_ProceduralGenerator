#pragma once
#include "City.h"
#include "Emergency.h"
#include "TextureManager.h"
namespace visual {
void initialize();
void shutdown();
void setDetailed(bool enabled);
bool detailed();
// Cached presentation only: category/facility identity comes from the original
// building, while its artwork may occupy a larger, clearance-checked envelope.
struct BuildingArtwork {
    city::Boundary property;
    city::Boundary bounds;
    Texture texture;
    bool rotated = false;
    int category = 0; // residential, commercial, office, hospital
};
BuildingArtwork layoutBuilding(const city::Building& footprint,
                              const city::Facilities& facilities,
                              const city::Boundary& envelope);
bool drawBuilding(const BuildingArtwork& artwork);
// Return false for missing assets so the renderer can keep its procedural fallback.
bool building(const city::Building& footprint,const city::Facilities& facilities);
bool tree(float x,float y,float radius);
}
