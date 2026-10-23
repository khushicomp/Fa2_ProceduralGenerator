#pragma once
#include "City.h"
#include "Emergency.h"
namespace visual {
struct BuildingArtwork;
// Same order as the read-only input buildings; valid after composition().
const BuildingArtwork& buildingPresentation(std::size_t index);
// Immediate-mode decoration only. No new simulation parcels/zones are stored.
void composition(const std::vector<city::Block>& blocks,
                 const std::vector<city::Building>& buildings,
                 const std::vector<city::Road>& roads,const city::Facilities& facilities,float pixelsPerUnit=1.0f);
}
