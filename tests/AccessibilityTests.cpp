#include "AccessibilityAnalyzer.h"
#include "CityGenerator.h"
#include "RoadGraphBuilder.h"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
city::RoadGraph graph(int count, std::initializer_list<city::GraphEdge> edges) {
    city::RoadGraph g;
    for (int i = 0; i < count; ++i) { g.nodes.push_back({i, {static_cast<float>(i), 0}}); }
    g.adjacency.resize(count);
    for (const auto& e : edges) {
        g.edges.push_back(e); g.adjacency[e.from].push_back({e.to, e.weight});
        g.adjacency[e.to].push_back({e.from, e.weight});
    }
    return g;
}
city::Facilities facilities(std::initializer_list<int> hospitals, int emergency = 0) {
    city::Facilities f;
    for (int node : hospitals) { f.hospitals.push_back({static_cast<int>(f.hospitals.size()), {0,0}, node}); }
    f.emergencies.push_back({0, {0,0}, emergency}); return f;
}
bool same(const city::Facilities& a, const city::Facilities& b) {
    auto equal = [](const auto& x, const auto& y) {
        if (x.size() != y.size()) { return false; }
        for (std::size_t i = 0; i < x.size(); ++i) {
            if (x[i].id != y[i].id || x[i].associatedGraphNodeId != y[i].associatedGraphNodeId ||
                x[i].position.x != y[i].position.x || x[i].position.y != y[i].position.y) { return false; }
        }
        return true;
    };
    return equal(a.hospitals, b.hospitals) && equal(a.emergencies, b.emergencies);
}
void validatePlacement(const city::Facilities& f, const city::RoadGraph& g) {
    std::set<std::pair<float,float>> positions;
    auto checkFacility = [&](const auto& facility) {
        const int node = facility.associatedGraphNodeId;
        check(node >= 0 && static_cast<std::size_t>(node) < g.nodes.size(), "Facility has invalid node");
        const auto p = facility.position;
        check(p.x >= city::boundary.min.x && p.x <= city::boundary.max.x &&
              p.y >= city::boundary.min.y && p.y <= city::boundary.max.y, "Facility outside city");
        check(positions.emplace(p.x,p.y).second, "Duplicate facility position");
        const auto target = g.nodes[node].position;
        const double chosen = std::hypot(static_cast<double>(p.x) - target.x, static_cast<double>(p.y) - target.y);
        for (const auto& n : g.nodes) {
            check(chosen <= std::hypot(static_cast<double>(p.x) - n.position.x,
                                      static_cast<double>(p.y) - n.position.y), "Attachment not nearest node");
        }
    };
    for (const auto& h : f.hospitals) { checkFacility(h); }
    for (const auto& e : f.emergencies) { checkFacility(e); }
}
void validateRoutes(const city::RoadGraph& g, const city::Facilities& f,
                    const std::vector<city::EmergencyRouteResult>& routes) {
    check(routes.size() == f.emergencies.size(), "Missing emergency result");
    for (std::size_t i = 0; i < routes.size(); ++i) {
        const auto& r = routes[i];
        check(r.emergencyId == f.emergencies[i].id, "Wrong emergency result");
        if (!r.reachable) {
            check(r.hospitalId == -1 && std::isinf(r.distance) && r.pathNodeIds.empty() &&
                  r.reachableHospitalIds.empty(), "Invalid unreachable result");
            continue;
        }
        int destination = -1;
        for (const auto& h : f.hospitals) { if (h.id == r.hospitalId) { destination = h.associatedGraphNodeId; } }
        check(destination >= 0 && !r.pathNodeIds.empty(), "Missing hospital/path");
        check(r.pathNodeIds.front() == f.emergencies[i].associatedGraphNodeId &&
              r.pathNodeIds.back() == destination, "Wrong route endpoints");
        double sum = 0;
        for (std::size_t j = 0; j < r.pathNodeIds.size(); ++j) {
            const int node = r.pathNodeIds[j];
            check(node >= 0 && static_cast<std::size_t>(node) < g.nodes.size(), "Invalid path node");
            if (j == 0) { continue; }
            bool found = false;
            for (const auto& edge : g.edges) {
                const int previous = r.pathNodeIds[j - 1];
                if ((edge.from == previous && edge.to == node) || (edge.to == previous && edge.from == node)) {
                    sum += edge.weight; found = true; break;
                }
            }
            check(found, "Route does not follow graph edge");
        }
        check(std::abs(sum - r.distance) < 1e-9, "Distance not equal to edge sum");
    }
}
}

int main() {
    try {
        const auto analyze = city::AccessibilityAnalyzer::analyze;
        const auto chain = graph(4, {{0,1,10},{1,2,20},{2,3,5}});
        const auto connected = analyze(chain, facilities({3}))[0];
        check(connected.reachable && connected.reachableHospitalIds == std::vector<int>{0}, "BFS connected");
        check(connected.pathNodeIds == std::vector<int>({0,1,2,3}), "Ordered path reconstruction");
        check(analyze(chain, facilities({2}))[0].distance == 30, "Simple path length");
        const auto disconnected = graph(4, {{0,1,10},{2,3,20}});
        const auto unreachable = analyze(disconnected, facilities({3}));
        check(!unreachable[0].reachable, "BFS disconnected");
        validateRoutes(disconnected, facilities({3}), unreachable);
        const auto mixed = analyze(disconnected, facilities({3,1}))[0];
        check(mixed.reachableHospitalIds == std::vector<int>{1} && mixed.hospitalId == 1,
              "Unreachable hospital not excluded");

        auto choice = graph(5, {{0,1,10},{1,2,10},{0,3,5},{3,4,50}});
        // H2 is geometrically closer, but its weighted road path is longer.
        choice.nodes[2].position = {100,0}; choice.nodes[4].position = {0.1f,0};
        auto f = facilities({2,4});
        f.hospitals[0].position = {100,0}; f.hospitals[1].position = {0.1f,0};
        const auto best = analyze(choice, f)[0];
        check(best.hospitalId == 0 && best.distance == 20 && best.pathNodeIds == std::vector<int>({0,1,2}),
              "Selected hospital by geometry instead of road distance");
        check(best.reachableHospitalIds.size() == 2, "Multiple reachable hospitals");
        // A cheaper alternative must update an already discovered node.
        const auto alternative = graph(4, {{0,1,50},{0,2,5},{2,1,5},{1,3,10},{2,3,40}});
        const auto alternateRoute = analyze(alternative, facilities({3}))[0];
        check(alternateRoute.distance == 20 && alternateRoute.pathNodeIds == std::vector<int>({0,2,1,3}),
              "Dijkstra relaxation/stale queue handling");
        const auto colocated = analyze(chain, facilities({0}))[0];
        check(colocated.distance == 0 && colocated.pathNodeIds == std::vector<int>{0}, "Same-node route");
        check(!analyze(chain, facilities({}))[0].reachable, "No hospitals");
        check(analyze({}, {}).empty(), "Empty graph");
        auto tieFacilities = facilities({1,2});
        std::swap(tieFacilities.hospitals[0], tieFacilities.hospitals[1]);
        check(analyze(graph(3, {{0,1,10},{0,2,10}}), tieFacilities)[0].hospitalId == 0, "Hospital tie break");
        auto invalid = facilities({99}); bool rejected = false;
        try { analyze(chain, invalid); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "Invalid facility node accepted");
        rejected = false;
        try { analyze(graph(2, {{0,1,-1}}), facilities({1})); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "Negative weight accepted");

        for (std::uint32_t seed = 0; seed < 50; ++seed) {
            for (float density : {0.0f, 0.1f, 0.5f, 1.0f}) {
                city::GenerationParameters p; p.seed = seed; p.roadDensity = density;
                const auto data = city::CityGenerator::generateCity(city::boundary, p);
                const auto g = city::RoadGraphBuilder::build(data.roads);
                const auto placed = city::placeFacilities(data, g, seed);
                check(same(placed, city::placeFacilities(data,g,seed)), "Nondeterministic facilities");
                validatePlacement(placed,g);
                if (density == 0) { check(placed.hospitals.empty() && placed.emergencies.empty(), "Empty-road placement"); }
                else { check(placed.hospitals.size() == 3 && placed.emergencies.size() == 4, "Default counts"); }
                const auto results = analyze(g, placed), repeat = analyze(g, placed);
                validateRoutes(g, placed, results);
                for (std::size_t i = 0; i < results.size(); ++i) {
                    check(results[i].pathNodeIds == repeat[i].pathNodeIds &&
                          results[i].hospitalId == repeat[i].hospitalId && results[i].distance == repeat[i].distance &&
                          results[i].reachableHospitalIds == repeat[i].reachableHospitalIds, "Nondeterministic analysis");
                }
            }
        }
        const auto sparse = city::RoadGraphBuilder::build({{{100,100},{200,100}}});
        const auto limited = city::placeFacilities({}, sparse, 0, {1,4});
        check(limited.hospitals.size() == 1 && limited.emergencies.size() == 1, "Limited fallback sites");
        validatePlacement(limited, sparse);
        city::GenerationParameters p;
        const auto data = city::CityGenerator::generateCity(city::boundary,p);
        const auto g = city::RoadGraphBuilder::build(data.roads);
        check(!same(city::placeFacilities(data,g,42), city::placeFacilities(data,g,43)), "Seed variation");
        const auto configured = city::placeFacilities(data,g,42,{2,5});
        check(configured.hospitals.size() == 2 && configured.emergencies.size() == 5, "Configurable counts");
        std::cout << "Passed BFS/Dijkstra fixtures and 50 seeds x 4 densities: placement, nearest attachment, "
                     "determinism, route edges, summed weights, disconnected and empty graphs.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
