#pragma once

#include <vector>

namespace city {

// Simulation coordinates, independent of OpenGL and window pixels.
struct Point {
    float x;
    float y;
};

struct Road {
    // Complete centerline, not yet split at crossings. Later graph construction
    // can split these segments without changing generation or rendering data.
    Point start;
    Point end;
};

struct Boundary {
    Point min;
    Point max;
};

// Buildable rectangle, already inset from road centerlines and city edges.
struct Block { Point min; Point max; };
struct Building { Point min; Point max; };

struct CityData {
    std::vector<Road> roads;
    std::vector<Block> blocks;
    std::vector<Building> buildings;
};

inline constexpr float roadMargin = 6.0f;
inline constexpr float minimumBuildingSize = 12.0f;
inline constexpr float buildingSpacing = 6.0f;

inline constexpr float worldWidth = 1000.0f;
inline constexpr float worldHeight = 800.0f;
inline constexpr Boundary boundary{{100.0f, 100.0f}, {900.0f, 700.0f}};

} // namespace city
