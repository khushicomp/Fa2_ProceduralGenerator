#include "RoadGraphBuilder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace city {
namespace {
using Coordinate = std::pair<float, float>;
bool horizontal(const Road& road) { return road.start.y == road.end.y; }
bool contains(const Road& road, Point point) {
    return point.x >= std::min(road.start.x, road.end.x) &&
           point.x <= std::max(road.start.x, road.end.x) &&
           point.y >= std::min(road.start.y, road.end.y) &&
           point.y <= std::max(road.start.y, road.end.y);
}
}

RoadGraph RoadGraphBuilder::build(const std::vector<Road>& roads) {
    std::map<Coordinate, int> ids;
    for (const Road& road : roads) {
        for (Point p : {road.start, road.end}) {
            if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
                throw std::invalid_argument("Road graph requires finite coordinates");
            }
        }
        if ((road.start.x == road.end.x) == (road.start.y == road.end.y)) {
            throw std::invalid_argument("Road graph requires nonzero axis-aligned roads");
        }
        ids.emplace(Coordinate{road.start.x, road.start.y}, 0);
        ids.emplace(Coordinate{road.end.x, road.end.y}, 0);
    }
    // No calculated floating-point intersection values: copy vertical.x and
    // horizontal.y exactly. Shared coordinates therefore have identical keys.
    // Exact keys deliberately preserve distinct, arbitrarily close roads.
    for (std::size_t i = 0; i < roads.size(); ++i) {
        for (std::size_t j = i + 1; j < roads.size(); ++j) {
            if (horizontal(roads[i]) == horizontal(roads[j])) { continue; }
            const Road& h = horizontal(roads[i]) ? roads[i] : roads[j];
            const Road& v = horizontal(roads[i]) ? roads[j] : roads[i];
            const Point crossing{v.start.x, h.start.y};
            if (contains(h, crossing) && contains(v, crossing)) {
                ids.emplace(Coordinate{crossing.x, crossing.y}, 0);
            }
        }
    }
    if (ids.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("Too many graph nodes for integer ids");
    }
    RoadGraph graph;
    // Lexicographic ordering gives stable ids, even if road input order changes.
    for (auto& entry : ids) {
        entry.second = static_cast<int>(graph.nodes.size());
        graph.nodes.push_back({entry.second, {entry.first.first, entry.first.second}});
    }
    std::set<std::pair<int, int>> connections;
    for (const Road& road : roads) {
        std::vector<int> onRoad;
        // Nodes are already x/y sorted: this is road order for both orientations.
        // Scanning all nodes also splits collinear overlaps at their endpoints.
        for (const auto& node : graph.nodes) {
            if (contains(road, node.position)) { onRoad.push_back(node.id); }
        }
        for (std::size_t i = 1; i < onRoad.size(); ++i) {
            connections.emplace(onRoad[i - 1], onRoad[i]);
        }
    }
    graph.adjacency.resize(graph.nodes.size());
    for (const auto& connection : connections) {
        const int from = connection.first, to = connection.second;
        const Point a = graph.nodes[from].position, b = graph.nodes[to].position;
        // Euclidean distance also works if diagonal geometry is supported later.
        const double distance = std::hypot(static_cast<double>(b.x) - a.x,
                                           static_cast<double>(b.y) - a.y);
        graph.edges.push_back({from, to, distance});
        graph.adjacency[from].push_back({to, distance});
        graph.adjacency[to].push_back({from, distance});
    }
    return graph;
}

} // namespace city
