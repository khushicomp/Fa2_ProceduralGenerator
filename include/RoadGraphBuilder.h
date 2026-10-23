#pragma once

#include "RoadGraph.h"

namespace city {

class RoadGraphBuilder {
public:
    // Accepts finite, nonzero, axis-aligned segments in either direction.
    // Includes every endpoint, even at the boundary or on an isolated road.
    // Throws invalid_argument for unsupported/invalid geometry.
    static RoadGraph build(const std::vector<Road>& roads);
};

} // namespace city
