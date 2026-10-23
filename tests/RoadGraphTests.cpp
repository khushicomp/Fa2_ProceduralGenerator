#include "RoadGraphBuilder.h"
#include "CityGenerator.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
int nodeAt(const city::RoadGraph& graph, float x, float y) {
    for (const auto& n : graph.nodes) { if (n.position.x == x && n.position.y == y) { return n.id; } }
    throw std::runtime_error("Missing node");
}
bool connected(const city::RoadGraph& graph, int a, int b) {
    for (const auto& n : graph.adjacency[a]) { if (n.node == b) { return true; } }
    return false;
}
void validate(const city::RoadGraph& graph) {
    check(graph.nodes.size() == graph.adjacency.size(), "Adjacency size");
    std::set<std::pair<float, float>> coordinates;
    for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
        const auto& n = graph.nodes[i];
        check(n.id == static_cast<int>(i), "Invalid node id");
        check(coordinates.emplace(n.position.x, n.position.y).second, "Duplicate node");
    }
    std::set<std::pair<int, int>> edges;
    for (const auto& e : graph.edges) {
        check(e.from >= 0 && e.to >= 0 && e.from < e.to &&
              static_cast<std::size_t>(e.to) < graph.nodes.size(), "Invalid edge endpoints");
        check(edges.emplace(e.from, e.to).second, "Duplicate edge");
        const auto a = graph.nodes[e.from].position, b = graph.nodes[e.to].position;
        check(e.weight > 0 && std::isfinite(e.weight) &&
              e.weight == std::hypot(static_cast<double>(b.x) - a.x, static_cast<double>(b.y) - a.y),
              "Incorrect edge weight");
        for (auto pair : {std::make_pair(e.from, e.to), std::make_pair(e.to, e.from)}) {
            int count = 0;
            for (const auto& n : graph.adjacency[pair.first]) {
                if (n.node == pair.second && n.weight == e.weight) { ++count; }
            }
            check(count == 1, "Undirected adjacency missing or duplicated");
        }
        for (const auto& n : graph.nodes) {
            if (n.id == e.from || n.id == e.to) { continue; }
            check(!(n.position.x >= std::min(a.x, b.x) && n.position.x <= std::max(a.x, b.x) &&
                    n.position.y >= std::min(a.y, b.y) && n.position.y <= std::max(a.y, b.y)),
                  "Edge skipped an intermediate node");
        }
    }
    std::size_t count = 0;
    for (const auto& neighbors : graph.adjacency) { count += neighbors.size(); }
    check(count == graph.edges.size() * 2, "Unexpected adjacency entries");
}
bool same(const city::RoadGraph& a, const city::RoadGraph& b) {
    if (a.nodes.size() != b.nodes.size() || a.edges.size() != b.edges.size()) { return false; }
    for (std::size_t i = 0; i < a.nodes.size(); ++i) {
        if (a.nodes[i].id != b.nodes[i].id || a.nodes[i].position.x != b.nodes[i].position.x ||
            a.nodes[i].position.y != b.nodes[i].position.y) { return false; }
    }
    for (std::size_t i = 0; i < a.edges.size(); ++i) {
        if (a.edges[i].from != b.edges[i].from || a.edges[i].to != b.edges[i].to ||
            a.edges[i].weight != b.edges[i].weight) { return false; }
    }
    return true;
}
}

int main() {
    try {
        using city::Road;
        const auto build = city::RoadGraphBuilder::build;
        std::vector<Road> cross{{{100,400},{900,400}}, {{500,100},{500,700}}};
        const auto graph = build(cross);
        validate(graph);
        check(graph.nodes.size() == 5 && graph.edges.size() == 4, "Cross topology");
        check(graph.adjacency[nodeAt(graph,500,400)].size() == 4, "Cross degree");
        cross.push_back(cross[0]); cross.push_back({{500,700},{500,100}});
        check(same(graph, build(cross)), "Duplicate/reversed road changed graph");

        const auto multiple = build({{{0,5},{10,5}}, {{2,0},{2,10}}, {{4,0},{4,10}}, {{8,0},{8,10}}});
        validate(multiple);
        check(multiple.nodes.size() == 11 && multiple.edges.size() == 10, "Multiple intersections");
        check(connected(multiple, nodeAt(multiple,2,5), nodeAt(multiple,4,5)), "Missing consecutive edge");
        check(!connected(multiple, nodeAt(multiple,2,5), nodeAt(multiple,8,5)), "Shortcut edge");
        const auto tee = build({{{0,0},{10,0}}, {{5,0},{5,5}}});
        validate(tee);
        check(tee.nodes.size() == 4 && tee.edges.size() == 3, "T junction");
        const auto overlap = build({{{0,0},{10,0}}, {{5,0},{15,0}}});
        validate(overlap);
        check(overlap.nodes.size() == 4 && overlap.edges.size() == 3, "Collinear overlap");
        validate(build({{{0,0},{1,0}}, {{2,0},{3,0}}}));
        check(build({}).nodes.empty(), "Empty graph");
        const auto close = build({{{0,0},{1,0}}, {{0,0.00001f},{1,0.00001f}}});
        check(close.nodes.size() == 4 && close.edges.size() == 2, "Nearby roads merged");
        for (auto road : {Road{{0,0},{0,0}}, Road{{0,0},{1,1}},
                          Road{{0,0},{std::numeric_limits<float>::infinity(),0}}}) {
            bool rejected = false;
            try { build({road}); } catch (const std::invalid_argument&) { rejected = true; }
            check(rejected, "Invalid road accepted");
        }
        for (std::uint32_t seed = 0; seed < 50; ++seed) {
            for (float density : {0.0f, 0.1f, 0.5f, 1.0f}) {
                city::GenerationParameters p; p.seed = seed; p.roadDensity = density;
                auto roads = city::CityGenerator::generate(city::boundary, p);
                const auto g = build(roads);
                validate(g);
                check(same(g, build(city::CityGenerator::generate(city::boundary, p))), "Nondeterministic graph");
                const auto h = std::count_if(roads.begin(), roads.end(), [](const Road& r) { return r.start.y == r.end.y; });
                const auto v = roads.size() - h;
                check(g.nodes.size() == h * v + 2 * roads.size(), "Generated node count");
                check(g.edges.size() == h * (v + 1) + v * (h + 1), "Generated edge count");
                for (const auto& e : g.edges) {
                    const auto a = g.nodes[e.from].position, b = g.nodes[e.to].position;
                    bool onRoad = false;
                    for (const auto& r : roads) {
                        onRoad |= a.x >= r.start.x && b.x <= r.end.x &&
                                  a.y >= r.start.y && b.y <= r.end.y;
                    }
                    check(onRoad, "Graph edge not on visible road");
                }
                std::reverse(roads.begin(), roads.end());
                for (auto& r : roads) { std::swap(r.start, r.end); }
                check(same(g, build(roads)), "Road ordering changed graph");
            }
        }
        std::cout << "Passed graph fixtures and 50 seeds x 4 densities: ids, weights, adjacency, "
                     "duplicates, consecutive segments, determinism, and road correspondence.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
