#include "CityOptimizer.h"
#include "AccessibilityAnalyzer.h"
#include "RoadGraphBuilder.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
bool sameRoads(const std::vector<city::Road>& a, const std::vector<city::Road>& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].start.x != b[i].start.x || a[i].start.y != b[i].start.y ||
            a[i].end.x != b[i].end.x || a[i].end.y != b[i].end.y) { return false; }
    }
    return true;
}
void fixedPositions(const city::Facilities& before, const city::Facilities& after) {
    auto equal = [](const auto& a, const auto& b) {
        check(a.size() == b.size(), "Facility count changed");
        for (std::size_t i = 0; i < a.size(); ++i) {
            check(a[i].id == b[i].id && a[i].position.x == b[i].position.x && a[i].position.y == b[i].position.y,
                  "Facility moved");
        }
    };
    equal(before.hospitals,after.hospitals); equal(before.emergencies,after.emergencies);
}
void validate(const city::OptimizationResult& result, const city::Boundary& bounds,
              const city::OptimizationParameters& p) {
    const auto& before = result.original; const auto& after = result.optimized;
    check(std::isfinite(after.score.finalScore) && after.score.finalScore >= before.score.finalScore - 1e-9 &&
          after.score.finalScore >= 0 && after.score.finalScore <= 100, "Optimization worsened score");
    fixedPositions(before.facilities,after.facilities);
    check(result.iterationsAttempted <= p.maxIterations &&
          result.iterationsAttempted == result.modificationsAccepted + result.modificationsRejected &&
          result.history.size() == static_cast<std::size_t>(result.iterationsAttempted), "Iteration bookkeeping");
    check(after.data.roads.size() == before.data.roads.size() + result.addedRoads.size() &&
          result.addedRoads.size() == static_cast<std::size_t>(result.modificationsAccepted), "Added road count");
    double best = before.score.finalScore;
    for (const auto& step : result.history) {
        check(std::abs(step.candidateScore - best - step.improvement) < 1e-9, "History delta");
        check(step.accepted == (step.improvement > p.epsilon), "Incorrect acceptance");
        if (step.accepted) { best = step.candidateScore; }
    }
    check(best == after.score.finalScore, "History not monotonic");
    if (!result.modificationsAccepted) { check(sameRoads(before.data.roads,after.data.roads), "No-improvement roads changed"); }
    else { check(after.graph.nodes.size() > before.graph.nodes.size(), "Accepted roads did not alter graph"); }
    for (const auto& road : result.addedRoads) {
        for (auto point : {road.start,road.end}) {
            check(point.x >= bounds.min.x && point.x <= bounds.max.x && point.y >= bounds.min.y && point.y <= bounds.max.y,
                  "Added road out of bounds");
        }
        const bool horizontal = road.start.y == road.end.y;
        auto clearance = [&](const auto& facility) {
            check(std::abs((horizontal ? facility.position.y : facility.position.x) -
                           (horizontal ? road.start.y : road.start.x)) >= p.facilityClearance, "Road crosses facility");
        };
        for (const auto& h : after.facilities.hospitals) { clearance(h); }
        for (const auto& e : after.facilities.emergencies) { clearance(e); }
        for (const auto& other : after.data.roads) {
            if (horizontal != (other.start.y == other.end.y)) { continue; }
            const float gap = std::abs((horizontal ? other.start.y : other.start.x) -
                                       (horizontal ? road.start.y : road.start.x));
            check(gap == 0 || gap >= p.minimumRoadSpacing, "Added road spacing");
        }
    }
    std::set<std::pair<int,int>> unique;
    for (std::size_t i = 0; i < after.graph.nodes.size(); ++i) { check(after.graph.nodes[i].id == static_cast<int>(i), "Node id"); }
    for (const auto& edge : after.graph.edges) {
        check(edge.from >= 0 && edge.to > edge.from && static_cast<std::size_t>(edge.to) < after.graph.nodes.size() &&
              edge.weight > 0 && std::isfinite(edge.weight), "Invalid optimized edge");
        check(unique.emplace(edge.from,edge.to).second, "Duplicate edge");
    }
    // Independent real pipeline recomputation, not the optimizer's cached score.
    const auto graph = city::RoadGraphBuilder::build(after.data.roads);
    const auto routes = city::AccessibilityAnalyzer::analyze(graph,after.facilities);
    const auto score = city::AccessibilityScorer::calculate(routes,bounds);
    check(score.finalScore == after.score.finalScore && routes.size() == after.routes.size(), "Score inconsistency");
    for (std::size_t i = 0; i < routes.size(); ++i) {
        const auto& r = routes[i];
        check(r.pathNodeIds == after.routes[i].pathNodeIds && r.distance == after.routes[i].distance, "Route inconsistency");
        if (!r.reachable) { check(r.pathNodeIds.empty(), "Unreachable path"); continue; }
        double sum = 0;
        for (std::size_t j = 0; j < r.pathNodeIds.size(); ++j) {
            const int node = r.pathNodeIds[j];
            check(node >= 0 && static_cast<std::size_t>(node) < graph.nodes.size(), "Path node");
            if (!j) { continue; }
            bool found = false;
            for (const auto& edge : graph.adjacency[r.pathNodeIds[j-1]]) {
                if (edge.node == node) { sum += edge.weight; found = true; break; }
            }
            check(found,"Path not on graph edge");
        }
        check(std::abs(sum-r.distance) < 1e-8,"Route distance sum");
    }
    for (const auto& b : after.data.buildings) {
        bool contained = false;
        for (const auto& block : after.data.blocks) {
            contained |= b.min.x >= block.min.x && b.max.x <= block.max.x &&
                         b.min.y >= block.min.y && b.max.y <= block.max.y;
        }
        check(contained,"Building outside block");
        for (const auto& r : after.data.roads) {
            check(r.start.y == r.end.y ? (b.max.y < r.start.y || b.min.y > r.start.y)
                                      : (b.max.x < r.start.x || b.min.x > r.start.x), "Building overlaps road");
        }
    }
}
city::OptimizationResult generated(std::uint32_t seed, float rd, float bd) {
    city::GenerationParameters generation; generation.seed = seed; generation.roadDensity = rd; generation.buildingDensity = bd;
    const auto data = city::CityGenerator::generateCity(city::boundary,generation);
    const auto graph = city::RoadGraphBuilder::build(data.roads);
    const auto facilities = city::placeFacilities(data,graph,seed);
    city::OptimizationParameters p;
    const auto result = city::CityOptimizer::optimize(data,city::boundary,facilities,generation,p);
    validate(result,city::boundary,p);
    check(sameRoads(result.original.data.roads,data.roads),"Original roads changed");
    fixedPositions(facilities,result.original.facilities);
    const auto repeat = city::CityOptimizer::optimize(data,city::boundary,facilities,generation,p);
    check(sameRoads(result.optimized.data.roads,repeat.optimized.data.roads) &&
          result.optimized.score.finalScore == repeat.optimized.score.finalScore &&
          result.history.size() == repeat.history.size(),"Nondeterministic optimization");
    for (std::size_t i = 0; i < result.history.size(); ++i) {
        check(result.history[i].candidateScore == repeat.history[i].candidateScore &&
              result.history[i].accepted == repeat.history[i].accepted,"Nondeterministic history");
    }
    return result;
}
}
int main() {
    try {
        city::GenerationParameters generation;
        city::OptimizationParameters p;
        const city::Boundary bounds{{0,0},{1000,1000}};
        const std::vector<city::Road> roads{{{0,200},{1000,200}},{{0,800},{1000,800}},{{100,0},{100,1000}}};
        const auto data = city::CityGenerator::rebuildFromRoads(bounds,roads,generation);
        city::Facilities fixed;
        fixed.hospitals.push_back({0,{570,790},0}); fixed.emergencies.push_back({0,{570,210},0});
        const auto improved = city::CityOptimizer::optimize(data,bounds,fixed,generation,p);
        validate(improved,bounds,p);
        check(improved.modificationsAccepted > 0 && improved.optimized.score.finalScore > improved.original.score.finalScore,
              "Known shortcut not accepted");
        check(improved.optimized.routes[0].distance < improved.original.routes[0].distance,"Known route not shortened");
        auto noHospital = fixed; noHospital.hospitals.clear();
        const auto rejected = city::CityOptimizer::optimize(data,bounds,noHospital,generation,p);
        check(rejected.modificationsRejected > 0 && rejected.modificationsAccepted == 0,"Non-improving candidate accepted");
        validate(rejected,bounds,p);
        p.maxIterations = 0;
        const auto zero = city::CityOptimizer::optimize(data,bounds,fixed,generation,p);
        check(zero.iterationsAttempted == 0 && sameRoads(zero.optimized.data.roads,roads),"Zero iteration budget");
        p.maxIterations = 1;
        check(city::CityOptimizer::optimize(data,bounds,fixed,generation,p).iterationsAttempted <= 1,"Iteration budget");
        p = {};
        auto sameNode = fixed; sameNode.hospitals[0].position = {571,210};
        const auto perfect = city::CityOptimizer::optimize(data,bounds,sameNode,generation,p);
        check(perfect.original.score.finalScore == 100 && perfect.iterationsAttempted == 0,"Perfect score early stop");
        p.minimumRoadSpacing = 100;
        auto blocked = fixed; blocked.hospitals[0].position.x = 550;
        check(city::CityOptimizer::optimize(data,bounds,blocked,generation,p).iterationsAttempted == 0,"Facility collision not filtered");
        p.minimumRoadSpacing = 10000;
        check(city::CityOptimizer::optimize(data,bounds,fixed,generation,p).iterationsAttempted == 0,"No-candidate early stop");
        p = {}; p.maxIterations = -1;
        bool invalid = false;
        try { city::CityOptimizer::optimize(data,bounds,fixed,generation,p); }
        catch (const std::invalid_argument&) { invalid = true; }
        check(invalid,"Invalid configuration accepted");
        for (std::uint32_t seed = 0; seed < 20; ++seed) {
            for (float rd : {0.0f,0.1f,0.5f,1.0f}) {
                for (float bd : {0.1f,0.5f}) { generated(seed,rd,bd); }
            }
        }
        std::cout << "Passed optimizer fixtures and 20 seeds x 4 road densities x 2 building densities.\n"
                  << std::fixed << std::setprecision(4);
        for (std::uint32_t seed : {7u,21u,42u,77u,99u}) {
            const auto r = generated(seed,0.5f,0.5f);
            std::cout << "Seed " << seed << " | Before " << r.original.score.finalScore << " | After "
                      << r.optimized.score.finalScore << " | Improvement "
                      << r.optimized.score.finalScore-r.original.score.finalScore << " | Accepted "
                      << r.modificationsAccepted << '\n';
        }
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
