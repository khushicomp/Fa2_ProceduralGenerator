#include "AccessibilityScorer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace city {
AccessibilityScore AccessibilityScorer::calculate(const std::vector<EmergencyRouteResult>& routes,
                                                  const Boundary& boundary) {
    if (!std::isfinite(boundary.min.x) || !std::isfinite(boundary.min.y) ||
        !std::isfinite(boundary.max.x) || !std::isfinite(boundary.max.y) ||
        boundary.max.x <= boundary.min.x || boundary.max.y <= boundary.min.y) {
        throw std::invalid_argument("Scoring requires finite positive boundary dimensions");
    }
    AccessibilityScore score;
    score.referenceDistance = std::hypot(static_cast<double>(boundary.max.x) - boundary.min.x,
                                         static_cast<double>(boundary.max.y) - boundary.min.y);
    score.totalEmergencyLocations = routes.size();
    for (const auto& route : routes) {
        if (!route.reachable) { continue; }
        if (!std::isfinite(route.distance) || route.distance < 0) {
            throw std::invalid_argument("Reachable road distances must be finite and nonnegative");
        }
        ++score.reachableEmergencyLocations;
        // Running mean avoids overflowing a sum of large finite distances.
        score.averageShortestRoadDistance +=
            (route.distance - score.averageShortestRoadDistance) / score.reachableEmergencyLocations;
        score.worstShortestRoadDistance = std::max(score.worstShortestRoadDistance, route.distance);
    }
    score.unreachableEmergencyLocations = score.totalEmergencyLocations - score.reachableEmergencyLocations;
    if (score.totalEmergencyLocations == 0) { return score; }
    score.reachabilityPercentage = 100.0 * static_cast<double>(score.reachableEmergencyLocations) /
                                   score.totalEmergencyLocations;
    score.reachabilityScore = score.reachabilityPercentage;
    if (score.reachableEmergencyLocations > 0) {
        // Normalization is centralized here; road distances exclude access connectors.
        score.distanceScore = std::clamp(100.0 * (1.0 - std::min(
            score.averageShortestRoadDistance / score.referenceDistance, 1.0)), 0.0, 100.0);
    }
    score.finalScore = std::clamp(REACHABILITY_WEIGHT * score.reachabilityScore +
                                  DISTANCE_WEIGHT * score.distanceScore, 0.0, 100.0);
    return score;
}
} // namespace city
