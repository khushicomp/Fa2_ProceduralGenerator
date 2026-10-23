#include "Emergency.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <set>
#include <stdexcept>

namespace city {
Facilities placeFacilities(const CityData& city, const RoadGraph& graph,
                           std::uint32_t seed, const FacilityParameters& parameters) {
    if (parameters.hospitalCount < 0 || parameters.emergencyCount < 0) {
        throw std::invalid_argument("Facility counts must be nonnegative");
    }
    Facilities result;
    if (graph.nodes.empty()) { return result; }
    std::mt19937 random(seed);
    std::vector<Point> sites;
    std::set<std::pair<float, float>> used;
    auto append = [&](std::vector<Point> candidates) {
        for (std::size_t i = candidates.size(); i > 1; --i) {
            std::swap(candidates[i - 1], candidates[random() % i]);
        }
        for (Point p : candidates) {
            if (used.emplace(p.x, p.y).second) { sites.push_back(p); }
        }
    };
    std::vector<Point> centers;
    for (const auto& b : city.buildings) {
        centers.push_back({b.min.x + (b.max.x - b.min.x) / 2, b.min.y + (b.max.y - b.min.y) / 2});
    }
    append(centers); centers.clear();
    for (const auto& b : city.blocks) {
        centers.push_back({b.min.x + (b.max.x - b.min.x) / 2, b.min.y + (b.max.y - b.min.y) / 2});
    }
    append(centers); centers.clear();
    for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
        const auto& node = graph.nodes[i];
        if (node.id != static_cast<int>(i) || !std::isfinite(node.position.x) || !std::isfinite(node.position.y)) {
            throw std::invalid_argument("Invalid graph node for facility placement");
        }
        centers.push_back(node.position);
    }
    append(centers);
    auto nearest = [&](Point p) {
        int best = -1;
        double distance = std::numeric_limits<double>::infinity();
        for (const auto& node : graph.nodes) {
            const double d = std::hypot(static_cast<double>(p.x) - node.position.x,
                                         static_cast<double>(p.y) - node.position.y);
            if (d < distance) { distance = d; best = node.id; }
        }
        return best; // Equal distances retain the lower node id.
    };
    std::size_t index = 0;
    for (int id = 0; id < parameters.hospitalCount && index < sites.size(); ++id, ++index) {
        result.hospitals.push_back({id, sites[index], nearest(sites[index])});
    }
    for (int id = 0; id < parameters.emergencyCount && index < sites.size(); ++id, ++index) {
        result.emergencies.push_back({id, sites[index], nearest(sites[index])});
    }
    return result;
}
} // namespace city
