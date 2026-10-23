#include "CityOptimizer.h"
#include "AccessibilityAnalyzer.h"
#include "RoadGraphBuilder.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace city {
namespace {
Facilities reattach(const Facilities& fixed, const RoadGraph& graph) {
    Facilities result = fixed;
    auto attach = [&](auto& facility) {
        double best = std::numeric_limits<double>::infinity();
        int id = -1;
        for (const auto& node : graph.nodes) {
            const double distance = std::hypot(static_cast<double>(facility.position.x) - node.position.x,
                                                static_cast<double>(facility.position.y) - node.position.y);
            if (distance < best) { best = distance; id = node.id; }
        }
        if (id < 0) { throw std::invalid_argument("Fixed facilities require road nodes"); }
        facility.associatedGraphNodeId = id;
    };
    for (auto& h : result.hospitals) { attach(h); }
    for (auto& e : result.emergencies) { attach(e); }
    return result;
}
CitySnapshot evaluate(const std::vector<Road>& roads, const Boundary& bounds, const Facilities& fixed) {
    CitySnapshot state;
    state.data.roads = roads;
    state.graph = RoadGraphBuilder::build(roads);
    state.facilities = reattach(fixed, state.graph);
    state.routes = AccessibilityAnalyzer::analyze(state.graph, state.facilities);
    state.score = AccessibilityScorer::calculate(state.routes, bounds);
    return state;
}
std::vector<Road> candidates(const CitySnapshot& state, const Boundary& bounds,
                             const OptimizationParameters& p, std::mt19937& random) {
    std::vector<float> xs{bounds.min.x, bounds.max.x}, ys{bounds.min.y, bounds.max.y};
    int horizontal = 0, vertical = 0;
    for (const auto& r : state.data.roads) {
        if (r.start.y == r.end.y) { ys.push_back(r.start.y); ++horizontal; }
        else { xs.push_back(r.start.x); ++vertical; }
    }
    std::vector<Road> proposals;
    auto propose = [&](std::vector<float>& coordinates, bool isHorizontal, int crossings) {
        if (crossings < 2) { return; } // Must link at least two perpendicular roads.
        std::sort(coordinates.begin(), coordinates.end());
        coordinates.erase(std::unique(coordinates.begin(), coordinates.end()), coordinates.end());
        for (std::size_t i = 1; i < coordinates.size(); ++i) {
            const float middle = coordinates[i - 1] + (coordinates[i] - coordinates[i - 1]) / 2;
            if (middle - coordinates[i - 1] < p.minimumRoadSpacing ||
                coordinates[i] - middle < p.minimumRoadSpacing) { continue; }
            auto clear = [&](const auto& facility) {
                return std::abs((isHorizontal ? facility.position.y : facility.position.x) - middle)
                       >= p.facilityClearance;
            };
            if (!std::all_of(state.facilities.hospitals.begin(), state.facilities.hospitals.end(), clear) ||
                !std::all_of(state.facilities.emergencies.begin(), state.facilities.emergencies.end(), clear)) { continue; }
            proposals.push_back(isHorizontal ? Road{{bounds.min.x,middle},{bounds.max.x,middle}}
                                              : Road{{middle,bounds.min.y},{middle,bounds.max.y}});
        }
    };
    // Full-span midpoint connectors preserve rectangular block subdivision.
    propose(ys, true, vertical); propose(xs, false, horizontal);
    for (std::size_t i = proposals.size(); i > 1; --i) {
        std::swap(proposals[i - 1], proposals[random() % i]);
    }
    return proposals;
}
}

OptimizationResult CityOptimizer::optimize(const CityData& original, const Boundary& bounds,
    const Facilities& fixedFacilities, const GenerationParameters& generation,
    const OptimizationParameters& parameters) {
    if (parameters.maxIterations < 0 || !std::isfinite(parameters.epsilon) || parameters.epsilon <= 0 ||
        !std::isfinite(parameters.minimumRoadSpacing) || parameters.minimumRoadSpacing <= 0 ||
        !std::isfinite(parameters.facilityClearance) || parameters.facilityClearance <= 0) {
        throw std::invalid_argument("Invalid optimization parameters");
    }
    // Scorer validates the boundary; the builder validates road coordinates.
    AccessibilityScorer::calculate({}, bounds);
    for (const auto& r : original.roads) {
        const bool h = r.start.y == r.end.y && r.start.y >= bounds.min.y && r.start.y <= bounds.max.y &&
                       std::min(r.start.x,r.end.x) == bounds.min.x && std::max(r.start.x,r.end.x) == bounds.max.x;
        const bool v = r.start.x == r.end.x && r.start.x >= bounds.min.x && r.start.x <= bounds.max.x &&
                       std::min(r.start.y,r.end.y) == bounds.min.y && std::max(r.start.y,r.end.y) == bounds.max.y;
        if (!h && !v) { throw std::invalid_argument("Optimizer requires full-boundary axis-aligned roads"); }
    }
    auto validatePosition = [&](const auto& f) {
        if (!std::isfinite(f.position.x) || !std::isfinite(f.position.y) ||
            f.position.x < bounds.min.x || f.position.x > bounds.max.x ||
            f.position.y < bounds.min.y || f.position.y > bounds.max.y) {
            throw std::invalid_argument("Fixed facility outside boundary");
        }
    };
    for (const auto& h : fixedFacilities.hospitals) { validatePosition(h); }
    for (const auto& e : fixedFacilities.emergencies) { validatePosition(e); }
    OptimizationResult result;
    result.original = evaluate(original.roads, bounds, fixedFacilities);
    result.original.data = original;
    result.optimized = result.original;
    std::mt19937 random(parameters.seed);
    auto proposals = candidates(result.optimized,bounds,parameters,random);
    std::size_t next = 0;
    while (result.iterationsAttempted < parameters.maxIterations && next < proposals.size() &&
           result.optimized.score.finalScore < 100.0 - parameters.epsilon) {
        const Road road = proposals[next++];
        auto roads = result.optimized.data.roads;
        roads.push_back(road);
        auto candidate = evaluate(roads, bounds, fixedFacilities);
        const double improvement = candidate.score.finalScore - result.optimized.score.finalScore;
        const bool accept = improvement > parameters.epsilon;
        result.history.push_back({road,candidate.score.finalScore,improvement,accept});
        ++result.iterationsAttempted;
        if (accept) {
            result.optimized = std::move(candidate);
            result.addedRoads.push_back(road);
            ++result.modificationsAccepted;
            // New gaps permit new proposals. Rejections may be reconsidered only
            // after a real improvement has changed the current network.
            proposals = candidates(result.optimized,bounds,parameters,random);
            next = 0;
        } else { ++result.modificationsRejected; }
    }
    if (result.modificationsAccepted) {
        result.optimized.data = CityGenerator::rebuildFromRoads(bounds,result.optimized.data.roads,generation);
    }
    return result;
}
} // namespace city
