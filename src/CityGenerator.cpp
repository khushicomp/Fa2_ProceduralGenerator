#include "CityGenerator.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>

namespace city {
namespace {

std::vector<float> positions(float low, float high, float spacing,
                             float density, std::mt19937& random) {
    // Each slot is 2*spacing wide. Jitter occupies only its central half,
    // leaving at least spacing between neighboring parallel centerlines.
    const double slotWidth = 2.0 * spacing;
    const double capacity = std::floor((static_cast<double>(high) - low) / slotWidth);
    if (capacity > 100000.0) {
        throw std::length_error("Too many road slots; increase minimumRoadSpacing");
    }
    const auto count = static_cast<std::size_t>(capacity);
    std::vector<std::size_t> slots(count);
    std::iota(slots.begin(), slots.end(), std::size_t{0});
    // Explicit shuffle and random-to-float mapping keep seed behavior independent
    // of standard-library distribution/shuffle implementations.
    for (std::size_t i = count; i > 1; --i) {
        std::swap(slots[i - 1], slots[random() % i]);
    }
    const auto selected = static_cast<std::size_t>(std::ceil(density * static_cast<double>(count)));
    std::vector<float> result;
    result.reserve(selected);
    for (std::size_t i = 0; i < selected; ++i) {
        const double unit = static_cast<double>(random() >> 8) / 16777216.0;
        const double coordinate = low + (slots[i] + 0.5) * slotWidth
                                      + (unit - 0.5) * spacing;
        result.push_back(static_cast<float>(coordinate));
    }
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace

std::vector<Road> CityGenerator::generate(const Boundary& bounds,
                                         const GenerationParameters& parameters) {
    if (!std::isfinite(bounds.min.x) || !std::isfinite(bounds.min.y) ||
        !std::isfinite(bounds.max.x) || !std::isfinite(bounds.max.y) ||
        bounds.min.x >= bounds.max.x || bounds.min.y >= bounds.max.y ||
        !std::isfinite(parameters.roadDensity) || parameters.roadDensity < 0.0f ||
        parameters.roadDensity > 1.0f || !std::isfinite(parameters.minimumRoadSpacing) ||
        parameters.minimumRoadSpacing <= 0.0f) {
        throw std::invalid_argument("Expected finite ordered bounds, density in [0,1], and positive spacing");
    }
    std::mt19937 random(parameters.seed);
    const auto horizontal = positions(bounds.min.y, bounds.max.y,
        parameters.minimumRoadSpacing, parameters.roadDensity, random);
    const auto vertical = positions(bounds.min.x, bounds.max.x,
        parameters.minimumRoadSpacing, parameters.roadDensity, random);
    std::vector<Road> roads;
    roads.reserve(horizontal.size() + vertical.size());
    // Reuse exact shared coordinates. Intersections are simply (vertical.x,
    // horizontal.y); graph construction and splitting are deliberately deferred.
    for (float y : horizontal) {
        roads.push_back({{bounds.min.x, y}, {bounds.max.x, y}});
    }
    for (float x : vertical) {
        roads.push_back({{x, bounds.min.y}, {x, bounds.max.y}});
    }
    return roads;
}

CityData CityGenerator::generateCity(const Boundary& bounds,
                                     const GenerationParameters& parameters) {
    return rebuildFromRoads(bounds, generate(bounds, parameters), parameters);
}

CityData CityGenerator::rebuildFromRoads(const Boundary& bounds, const std::vector<Road>& roads,
                                        const GenerationParameters& parameters) {
    if (!std::isfinite(parameters.buildingDensity) || parameters.buildingDensity < 0.1f ||
        parameters.buildingDensity > 1.0f) {
        throw std::invalid_argument("Expected building density in [0.1,1]");
    }
    CityData result;
    result.roads = roads;
    std::vector<float> xs{bounds.min.x, bounds.max.x};
    std::vector<float> ys{bounds.min.y, bounds.max.y};
    for (const auto& road : result.roads) {
        if (road.start.x == road.end.x) { xs.push_back(road.start.x); }
        else { ys.push_back(road.start.y); }
    }
    for (auto* coordinates : {&xs, &ys}) {
        std::sort(coordinates->begin(), coordinates->end());
        coordinates->erase(std::unique(coordinates->begin(), coordinates->end()), coordinates->end());
    }
    if (static_cast<double>(xs.size() - 1) * (ys.size() - 1) > 100000.0) {
        throw std::length_error("Too many blocks for this prototype");
    }
    // Consecutive separators form cells. Inset all edges so neither buildings
    // nor their outlines touch roads. Discard cells too small for one building.
    for (std::size_t y = 1; y < ys.size(); ++y) {
        for (std::size_t x = 1; x < xs.size(); ++x) {
            Block block{{xs[x - 1] + roadMargin, ys[y - 1] + roadMargin},
                        {xs[x] - roadMargin, ys[y] - roadMargin}};
            if (block.max.x - block.min.x >= minimumBuildingSize + buildingSpacing &&
                block.max.y - block.min.y >= minimumBuildingSize + buildingSpacing) {
                result.blocks.push_back(block);
            }
        }
    }

    // Separate random stream: buildings never change the existing road layout.
    std::mt19937 random(parameters.seed);
    auto unit = [&random]() { return static_cast<float>(random() >> 8) / 16777216.0f; };
    std::size_t totalLots = 0;
    for (const Block& block : result.blocks) {
        const float width = block.max.x - block.min.x;
        const float height = block.max.y - block.min.y;
        // Approximately 40-unit lots, with one lot for a smaller valid block.
        const double nx = std::max(1.0, std::floor(width / 40.0));
        const double ny = std::max(1.0, std::floor(height / 40.0));
        if (nx * ny > 1000000.0 - static_cast<double>(totalLots)) {
            throw std::length_error("Too many building lots for this prototype");
        }
        const int columns = static_cast<int>(nx);
        const int rows = static_cast<int>(ny);
        totalLots += static_cast<std::size_t>(columns) * rows;
        const float lotWidth = width / columns;
        const float lotHeight = height / rows;
        std::vector<Building> candidates;
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < columns; ++column) {
                const float w = minimumBuildingSize + unit() * (lotWidth - buildingSpacing - minimumBuildingSize);
                const float h = minimumBuildingSize + unit() * (lotHeight - buildingSpacing - minimumBuildingSize);
                const float x = block.min.x + column * lotWidth + buildingSpacing / 2
                              + unit() * (lotWidth - buildingSpacing - w);
                const float y = block.min.y + row * lotHeight + buildingSpacing / 2
                              + unit() * (lotHeight - buildingSpacing - h);
                candidates.push_back({{x, y}, {x + w, y + h}});
            }
        }
        for (std::size_t i = candidates.size(); i > 1; --i) {
            std::swap(candidates[i - 1], candidates[random() % i]);
        }
        // Generate all candidates before selecting a prefix: density changes
        // add/remove buildings without moving the retained footprints.
        const auto count = static_cast<std::size_t>(std::ceil(parameters.buildingDensity *
                                                             static_cast<double>(candidates.size())));
        result.buildings.insert(result.buildings.end(), candidates.begin(), candidates.begin() + count);
    }
    return result;
}

} // namespace city
