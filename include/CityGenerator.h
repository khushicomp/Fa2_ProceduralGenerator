#pragma once

#include "City.h"
#include <cstdint>
#include <vector>

namespace city {

struct GenerationParameters {
    float roadDensity = 0.5f;       // [0,1]; zero produces no roads.
    float minimumRoadSpacing = 40.0f; // World units between parallel roads.
    std::uint32_t seed = 42;
    float buildingDensity = 0.5f; // [0.1,1]; independent of road generation.
};

// Geometry only: no window, rendering, or graph dependencies.
class CityGenerator {
public:
    // Throws invalid_argument for invalid bounds/parameters, length_error for
    // more than 100,000 available slots per axis (a prototype resource limit).
    static std::vector<Road> generate(const Boundary& bounds,
                                     const GenerationParameters& parameters);
    // Rebuild all derived geometry from the current roads and parameters.
    static CityData generateCity(const Boundary& bounds,
                                 const GenerationParameters& parameters);
    // Reuse the existing subdivision for full-boundary axis-aligned roads.
    static CityData rebuildFromRoads(const Boundary& bounds, const std::vector<Road>& roads,
                                     const GenerationParameters& parameters);
};

} // namespace city
