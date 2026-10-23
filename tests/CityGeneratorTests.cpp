#include "CityGenerator.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

bool equal(const std::vector<city::Road>& a, const std::vector<city::Road>& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].start.x != b[i].start.x || a[i].start.y != b[i].start.y ||
            a[i].end.x != b[i].end.x || a[i].end.y != b[i].end.y) { return false; }
    }
    return true;
}

void checkGeometry(const std::vector<city::Road>& roads, city::Boundary bounds, float spacing) {
    for (std::size_t i = 0; i < roads.size(); ++i) {
        const auto& r = roads[i];
        for (auto p : {r.start, r.end}) {
            require(std::isfinite(p.x) && std::isfinite(p.y), "Nonfinite coordinate");
            require(p.x >= bounds.min.x && p.x <= bounds.max.x &&
                    p.y >= bounds.min.y && p.y <= bounds.max.y, "Road outside boundary");
        }
        const bool horizontal = r.start.y == r.end.y;
        require(horizontal != (r.start.x == r.end.x), "Road diagonal or zero length");
        require(horizontal ? (r.start.x == bounds.min.x && r.end.x == bounds.max.x)
                           : (r.start.y == bounds.min.y && r.end.y == bounds.max.y),
                "Road must span boundary for natural crossings");
        for (std::size_t j = 0; j < i; ++j) {
            const auto& other = roads[j];
            if (horizontal == (other.start.y == other.end.y)) {
                const float distance = horizontal ? std::abs(r.start.y - other.start.y)
                                                  : std::abs(r.start.x - other.start.x);
                require(distance >= spacing, "Parallel roads too close or duplicated");
            }
        }
    }
}
} // namespace

int main() {
    try {
        city::GenerationParameters parameters;
        const auto baseline = city::CityGenerator::generate(city::boundary, parameters);
        require(!baseline.empty(), "Default layout empty");
        for (std::uint32_t seed = 0; seed < 100; ++seed) {
            parameters.seed = seed;
            const auto a = city::CityGenerator::generate(city::boundary, parameters);
            require(equal(a, city::CityGenerator::generate(city::boundary, parameters)),
                    "Same seed produced different coordinates");
            if (seed != 42) { require(!equal(a, baseline), "Different seed matched baseline"); }
            std::size_t previous = 0;
            for (int density = 0; density <= 10; ++density) {
                auto p = parameters;
                p.roadDensity = density / 10.0f;
                const auto roads = city::CityGenerator::generate(city::boundary, p);
                checkGeometry(roads, city::boundary, p.minimumRoadSpacing);
                require(roads.size() >= previous, "Density decreased road count");
                if (density == 0) { require(roads.empty(), "Zero density not empty"); }
                previous = roads.size();
            }
            require(previous > a.size(), "Full density did not increase road count");
        }
        for (float spacing : {10.0f, 37.5f, 100.0f, 1000.0f}) {
            parameters.minimumRoadSpacing = spacing;
            parameters.roadDensity = 1.0f;
            const city::Boundary shifted{{-330.0f, -210.0f}, {210.0f, 370.0f}};
            checkGeometry(city::CityGenerator::generate(shifted, parameters), shifted, spacing);
        }
        require(city::CityGenerator::generate(city::boundary, parameters).empty(),
                "Oversized spacing should produce no roads");
        auto invalid = [](city::Boundary bounds, city::GenerationParameters p) {
            try { city::CityGenerator::generate(bounds, p); }
            catch (const std::invalid_argument&) { return; }
            throw std::runtime_error("Invalid input accepted");
        };
        parameters = {};
        for (float spacing : {0.0f, -1.0f, std::numeric_limits<float>::infinity()}) {
            auto p = parameters; p.minimumRoadSpacing = spacing; invalid(city::boundary, p);
        }
        for (float density : {-0.1f, 1.1f, std::numeric_limits<float>::quiet_NaN()}) {
            auto p = parameters; p.roadDensity = density; invalid(city::boundary, p);
        }
        invalid({{1, 1}, {1, 3}}, parameters);
        invalid({{2, 1}, {1, 3}}, parameters);
        std::cout << "Passed: 100 seeds, exact repeatability, seed variation, 11 density levels, "
                     "bounds, alignment, spacing, duplicates, and invalid inputs.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
