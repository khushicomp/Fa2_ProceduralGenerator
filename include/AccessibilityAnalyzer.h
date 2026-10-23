#pragma once
#include "Emergency.h"

namespace city {
class AccessibilityAnalyzer {
public:
    // Explicit BFS reachability, then priority-queue Dijkstra per emergency.
    // Requires nonnegative finite adjacency weights and index-based node ids.
    // Invalid graph/facility data throws invalid_argument rather than indexing it.
    static std::vector<EmergencyRouteResult> analyze(const RoadGraph& graph,
                                                    const Facilities& facilities);
};
} // namespace city
