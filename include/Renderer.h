#pragma once

#include "City.h"
#include "RoadGraph.h"
#include "Emergency.h"
#include <vector>

namespace renderer {

// Call only with a current OpenGL context and an initialized GLAD loader.
void resize(int framebufferWidth, int framebufferHeight);
void resizeRegion(int x, int y, int width, int height);
void draw(const city::Boundary& boundary, const std::vector<city::Road>& roads,
          const std::vector<city::Building>& buildings = {},
          const std::vector<city::Road>& addedRoads = {},
          const std::vector<city::Block>& blocks = {},
          const city::Facilities& facilities = {});
void drawGraph(const city::RoadGraph& graph);
void drawEmergency(const city::RoadGraph& graph, const city::Facilities& facilities,
                   const city::EmergencyRouteResult* selected);

} // namespace renderer
