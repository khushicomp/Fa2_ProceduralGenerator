#include "CityGenerator.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) {
    if (!ok) { throw std::runtime_error(message); }
}
template<class T> bool same(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].min.x != b[i].min.x || a[i].min.y != b[i].min.y ||
            a[i].max.x != b[i].max.x || a[i].max.y != b[i].max.y) { return false; }
    }
    return true;
}
void geometry(const city::CityData& data) {
    for (const auto& block : data.blocks) {
        check(block.min.x >= city::boundary.min.x + city::roadMargin &&
              block.max.x <= city::boundary.max.x - city::roadMargin &&
              block.min.y >= city::boundary.min.y + city::roadMargin &&
              block.max.y <= city::boundary.max.y - city::roadMargin, "Block outside city");
        check(block.max.x - block.min.x >= 18 && block.max.y - block.min.y >= 18, "Tiny block");
    }
    for (std::size_t i = 0; i < data.buildings.size(); ++i) {
        const auto& b = data.buildings[i];
        check(b.max.x - b.min.x >= city::minimumBuildingSize - 0.001f &&
              b.max.y - b.min.y >= city::minimumBuildingSize - 0.001f, "Tiny building");
        int containers = 0;
        for (const auto& block : data.blocks) {
            if (b.min.x >= block.min.x && b.max.x <= block.max.x &&
                b.min.y >= block.min.y && b.max.y <= block.max.y) { ++containers; }
        }
        check(containers == 1, "Building not inside exactly one block");
        for (const auto& road : data.roads) {
            if (road.start.x == road.end.x) {
                check(b.max.x <= road.start.x - city::roadMargin ||
                      b.min.x >= road.start.x + city::roadMargin, "Building overlaps vertical road");
            } else {
                check(b.max.y <= road.start.y - city::roadMargin ||
                      b.min.y >= road.start.y + city::roadMargin, "Building overlaps horizontal road");
            }
        }
        for (std::size_t j = 0; j < i; ++j) {
            const auto& a = data.buildings[j];
            const float gap = city::buildingSpacing - 0.001f;
            check(b.min.x >= a.max.x + gap || a.min.x >= b.max.x + gap ||
                  b.min.y >= a.max.y + gap || a.min.y >= b.max.y + gap,
                  "Buildings overlap or lack spacing");
        }
    }
}
}

int main() {
    try {
        city::GenerationParameters p;
        const auto baseline = city::CityGenerator::generateCity(city::boundary, p);
        for (std::uint32_t seed = 0; seed < 20; ++seed) {
            p.seed = seed;
            for (float roadDensity : {0.0f, 0.3f, 0.7f, 1.0f}) {
                p.roadDensity = roadDensity;
                std::size_t previous = 0;
                for (float density : {0.1f, 0.5f, 1.0f}) {
                    p.buildingDensity = density;
                    const auto data = city::CityGenerator::generateCity(city::boundary, p);
                    const auto repeat = city::CityGenerator::generateCity(city::boundary, p);
                    check(same(data.blocks, repeat.blocks) && same(data.buildings, repeat.buildings),
                          "City is not deterministic");
                    geometry(data);
                    check(data.buildings.size() >= previous, "Density decreased building count");
                    if (roadDensity == 0.0f) {
                        check(data.buildings.size() > previous, "Density did not increase count in large block");
                    }
                    previous = data.buildings.size();
                    std::vector<float> xs{city::boundary.min.x, city::boundary.max.x};
                    std::vector<float> ys{city::boundary.min.y, city::boundary.max.y};
                    for (const auto& r : data.roads) {
                        if (r.start.y == r.end.y) { ys.push_back(r.start.y); }
                        else { xs.push_back(r.start.x); }
                    }
                    std::sort(xs.begin(), xs.end()); std::sort(ys.begin(), ys.end());
                    std::size_t validColumns = 0, validRows = 0;
                    for (std::size_t i = 1; i < xs.size(); ++i) {
                        validColumns += xs[i] - xs[i - 1] >= 2 * city::roadMargin + 18;
                    }
                    for (std::size_t i = 1; i < ys.size(); ++i) {
                        validRows += ys[i] - ys[i - 1] >= 2 * city::roadMargin + 18;
                    }
                    check(data.blocks.size() == validRows * validColumns,
                          "Blocks not reconstructed from roads");
                    if (roadDensity == 0) { check(data.blocks.size() == 1, "No-road city needs one block"); }
                }
            }
        }
        p = {}; p.seed = 43;
        check(!same(baseline.buildings, city::CityGenerator::generateCity(city::boundary, p).buildings),
              "Different seed did not change buildings");
        p = {};
        check(city::CityGenerator::generateCity({{0, 0}, {20, 20}}, p).blocks.empty(), "Tiny block retained");
        for (float density : {0.0f, 1.1f, INFINITY, NAN}) {
            p.buildingDensity = density;
            bool rejected = false;
            try { city::CityGenerator::generateCity(city::boundary, p); }
            catch (const std::invalid_argument&) { rejected = true; }
            check(rejected, "Invalid density accepted");
        }
        std::cout << "Passed: 20 seeds, 4 road densities, 3 building densities; containment, road clearance, "
                     "building spacing, repeatability, count changes, block reconstruction, and invalid inputs.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
