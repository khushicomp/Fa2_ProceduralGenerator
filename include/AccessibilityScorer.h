#pragma once

#include "Emergency.h"
#include <cstddef>

namespace city {
// Connection to a hospital takes priority over route efficiency.
inline constexpr double REACHABILITY_WEIGHT = 0.60;
inline constexpr double DISTANCE_WEIGHT = 0.40;

struct AccessibilityScore {
    std::size_t totalEmergencyLocations = 0;
    std::size_t reachableEmergencyLocations = 0;
    std::size_t unreachableEmergencyLocations = 0;
    double reachabilityPercentage = 0;
    // Zero when no incident is reachable; report these metrics as N/A then.
    double averageShortestRoadDistance = 0;
    double worstShortestRoadDistance = 0;
    double referenceDistance = 0;
    double reachabilityScore = 0;
    double distanceScore = 0;
    double finalScore = 0;
};

class AccessibilityScorer {
public:
    // One Phase 3 result per incident. Unreachable distances are ignored.
    // Rejects invalid boundaries or negative/nonfinite reachable distances.
    static AccessibilityScore calculate(const std::vector<EmergencyRouteResult>& routes,
                                        const Boundary& boundary);
};
} // namespace city
