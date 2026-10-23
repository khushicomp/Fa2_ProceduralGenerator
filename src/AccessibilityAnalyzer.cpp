#include "AccessibilityAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <set>
#include <stdexcept>

namespace city {
std::vector<EmergencyRouteResult> AccessibilityAnalyzer::analyze(const RoadGraph& graph,
                                                               const Facilities& facilities) {
    const auto count = graph.nodes.size();
    auto valid = [count](int id) { return id >= 0 && static_cast<std::size_t>(id) < count; };
    if (graph.adjacency.size() != count) { throw std::invalid_argument("Invalid adjacency size"); }
    for (std::size_t i = 0; i < count; ++i) {
        if (graph.nodes[i].id != static_cast<int>(i)) { throw std::invalid_argument("Invalid node id"); }
        for (const auto& neighbor : graph.adjacency[i]) {
            if (!valid(neighbor.node) || !std::isfinite(neighbor.weight) || neighbor.weight < 0) {
                throw std::invalid_argument("Invalid adjacency entry");
            }
        }
    }
    std::set<int> hospitalIds, emergencyIds;
    for (const auto& h : facilities.hospitals) {
        if (h.id < 0 || !valid(h.associatedGraphNodeId) || !hospitalIds.insert(h.id).second) {
            throw std::invalid_argument("Invalid hospital");
        }
    }
    for (const auto& e : facilities.emergencies) {
        if (e.id < 0 || !valid(e.associatedGraphNodeId) || !emergencyIds.insert(e.id).second) {
            throw std::invalid_argument("Invalid emergency");
        }
    }
    std::vector<EmergencyRouteResult> results;
    for (const auto& emergency : facilities.emergencies) {
        const int start = emergency.associatedGraphNodeId;
        EmergencyRouteResult result;
        result.emergencyId = emergency.id;
        // BFS explicitly demonstrates connectivity independently of edge weights.
        std::vector<bool> visited(count, false);
        std::queue<int> queue;
        visited[start] = true; queue.push(start);
        while (!queue.empty()) {
            const int current = queue.front(); queue.pop();
            for (const auto& neighbor : graph.adjacency[current]) {
                if (!visited[neighbor.node]) {
                    visited[neighbor.node] = true; queue.push(neighbor.node);
                }
            }
        }
        for (const auto& hospital : facilities.hospitals) {
            if (visited[hospital.associatedGraphNodeId]) { result.reachableHospitalIds.push_back(hospital.id); }
        }
        std::sort(result.reachableHospitalIds.begin(), result.reachableHospitalIds.end());
        if (result.reachableHospitalIds.empty()) { results.push_back(result); continue; }

        std::vector<double> distances(count, std::numeric_limits<double>::infinity());
        std::vector<int> previous(count, -1);
        using Entry = std::pair<double, int>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pending;
        distances[start] = 0; pending.emplace(0, start);
        while (!pending.empty()) {
            const auto [distance, current] = pending.top(); pending.pop();
            if (distance != distances[current]) { continue; } // Stale queue entry.
            for (const auto& neighbor : graph.adjacency[current]) {
                const double candidate = distance + neighbor.weight;
                if (candidate < distances[neighbor.node]) {
                    distances[neighbor.node] = candidate;
                    previous[neighbor.node] = current;
                    pending.emplace(candidate, neighbor.node);
                }
            }
        }
        int destination = -1;
        for (const auto& hospital : facilities.hospitals) {
            const int node = hospital.associatedGraphNodeId;
            if (!visited[node]) { continue; }
            if (distances[node] < result.distance ||
                (distances[node] == result.distance && hospital.id < result.hospitalId)) {
                result.distance = distances[node]; result.hospitalId = hospital.id; destination = node;
            }
        }
        if (destination < 0) { throw std::overflow_error("Reachable road distance exceeds double range"); }
        result.reachable = true;
        // Follow Dijkstra predecessors from hospital back to the emergency.
        for (int node = destination; node != -1; node = previous[node]) { result.pathNodeIds.push_back(node); }
        std::reverse(result.pathNodeIds.begin(), result.pathNodeIds.end());
        results.push_back(result);
    }
    return results;
}
} // namespace city
