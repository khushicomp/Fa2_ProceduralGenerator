#pragma once

#include "RoadGraph.h"
#include <cstdint>
#include <limits>

namespace city {
struct Hospital { int id; Point position; int associatedGraphNodeId; };
struct EmergencyLocation { int id; Point position; int associatedGraphNodeId; };
struct Facilities {
    std::vector<Hospital> hospitals;
    std::vector<EmergencyLocation> emergencies;
};
struct FacilityParameters { int hospitalCount = 3; int emergencyCount = 4; };

// Prefer building centers, then block centers, then unused graph positions.
// Empty graphs produce no facilities; insufficient sites reduce actual counts.
Facilities placeFacilities(const CityData& city, const RoadGraph& graph,
                           std::uint32_t seed, const FacilityParameters& parameters = {});

struct EmergencyRouteResult {
    int emergencyId = -1;
    bool reachable = false;
    int hospitalId = -1;
    double distance = std::numeric_limits<double>::infinity();
    std::vector<int> reachableHospitalIds;
    std::vector<int> pathNodeIds;
};
} // namespace city
