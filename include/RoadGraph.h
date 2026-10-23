#pragma once

#include "City.h"
#include <vector>

namespace city {

struct GraphNode { int id; Point position; };
struct GraphEdge { int from; int to; double weight; };
struct GraphNeighbor { int node; double weight; };

struct RoadGraph {
    // Node ids are indices into nodes and adjacency. Each undirected edge is
    // stored once in edges, and twice (one per direction) in adjacency.
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
    std::vector<std::vector<GraphNeighbor>> adjacency;
};

} // namespace city
