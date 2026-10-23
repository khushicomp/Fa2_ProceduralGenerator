#pragma once

#include "AccessibilityScorer.h"
#include "CityGenerator.h"

namespace city {
struct OptimizationParameters {
    int maxIterations = 100;
    double epsilon = 1e-6;
    float minimumRoadSpacing = 40.0f;
    float facilityClearance = 10.0f;
    std::uint32_t seed = 42;
};
struct CitySnapshot {
    CityData data;
    RoadGraph graph;
    Facilities facilities;
    std::vector<EmergencyRouteResult> routes;
    AccessibilityScore score;
};
struct OptimizationStep {
    Road candidate;
    double candidateScore;
    double improvement; // Relative to the best state before this attempt.
    bool accepted;
};
struct OptimizationResult {
    CitySnapshot original;
    CitySnapshot optimized;
    int iterationsAttempted = 0;
    int modificationsAccepted = 0;
    int modificationsRejected = 0;
    std::vector<OptimizationStep> history;
    std::vector<Road> addedRoads;
};
class CityOptimizer {
public:
    // Add-only search for the current full-boundary road generator. Facilities
    // are copied and reattached, never placed again. Caller input is unchanged.
    static OptimizationResult optimize(const CityData& original, const Boundary& bounds,
        const Facilities& fixedFacilities, const GenerationParameters& generation,
        const OptimizationParameters& parameters = {});
};
} // namespace city
