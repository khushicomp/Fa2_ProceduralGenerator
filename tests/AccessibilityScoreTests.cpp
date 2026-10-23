#include "AccessibilityScorer.h"
#include "AccessibilityAnalyzer.h"
#include "CityGenerator.h"
#include "RoadGraphBuilder.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
void close(double a, double b) { check(std::abs(a-b) < 1e-9, "Unexpected metric"); }
std::vector<city::EmergencyRouteResult> routes(int total, int reachable, double distance) {
    std::vector<city::EmergencyRouteResult> result(total);
    for (int i = 0; i < total; ++i) {
        result[i].emergencyId = i;
        if (i < reachable) { result[i].reachable = true; result[i].distance = distance; }
    }
    return result;
}
city::AccessibilityScore generated(std::uint32_t seed, float roadDensity, float buildingDensity) {
    city::GenerationParameters p;
    p.seed = seed; p.roadDensity = roadDensity; p.buildingDensity = buildingDensity;
    const auto data = city::CityGenerator::generateCity(city::boundary, p);
    const auto graph = city::RoadGraphBuilder::build(data.roads);
    const auto facilities = city::placeFacilities(data, graph, seed);
    const auto results = city::AccessibilityAnalyzer::analyze(graph, facilities);
    const auto score = city::AccessibilityScorer::calculate(results, city::boundary);
    std::size_t reachable = 0;
    double sum = 0, worst = 0;
    for (const auto& r : results) {
        if (r.reachable) { ++reachable; sum += r.distance; worst = std::max(worst,r.distance); }
    }
    check(score.totalEmergencyLocations == results.size(), "Total count");
    check(score.reachableEmergencyLocations == reachable && reachable <= results.size(), "Reachable count");
    check(score.unreachableEmergencyLocations == results.size() - reachable, "Unreachable count");
    close(score.averageShortestRoadDistance, reachable ? sum / reachable : 0);
    close(score.worstShortestRoadDistance, worst);
    for (double metric : {score.finalScore, score.distanceScore, score.reachabilityScore, score.reachabilityPercentage}) {
        check(std::isfinite(metric) && metric >= 0 && metric <= 100, "Score out of range");
    }
    return score;
}
}

int main() {
    try {
        const auto calculate = city::AccessibilityScorer::calculate;
        const city::Boundary bounds{{0,0},{300,400}}; // Known diagonal: 500.
        const auto full = calculate(routes(5,5,250),bounds);
        close(full.referenceDistance,500); close(full.reachabilityScore,100);
        const auto partial = calculate(routes(5,4,250),bounds);
        close(partial.reachabilityScore,80); close(partial.reachabilityPercentage,80);
        close(partial.distanceScore,50); close(partial.finalScore,68);
        close(partial.averageShortestRoadDistance,250); close(partial.worstShortestRoadDistance,250);
        check(partial.finalScore < full.finalScore, "Unreachable incident did not lower score");
        const auto none = calculate(routes(5,0,0),bounds);
        close(none.reachabilityScore,0); close(none.distanceScore,0); close(none.finalScore,0);
        close(none.averageShortestRoadDistance,0); close(none.worstShortestRoadDistance,0);
        check(none.unreachableEmergencyLocations == 5, "No reachability counts");
        const auto empty = calculate({},bounds);
        close(empty.finalScore,0); close(empty.distanceScore,0); close(empty.reachabilityPercentage,0);
        check(empty.totalEmergencyLocations == 0, "Empty count");
        check(calculate(routes(5,5,100),bounds).distanceScore > full.distanceScore, "Shorter route not rewarded");
        close(calculate(routes(1,1,0),bounds).finalScore,100);
        for (double d : {500.0,1000.0,std::numeric_limits<double>::max()}) {
            const auto s = calculate(routes(2,2,d),bounds);
            close(s.distanceScore,0); close(s.finalScore,60);
            check(std::isfinite(s.averageShortestRoadDistance), "Average overflow");
        }
        auto varied = routes(3,2,100); varied[1].distance = 300;
        const auto variedScore = calculate(varied,bounds);
        close(variedScore.averageShortestRoadDistance,200); close(variedScore.worstShortestRoadDistance,300);
        close(calculate(routes(5,4,500),{{-100,-100},{500,700}}).distanceScore,50);
        for (double bad : {-1.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
            bool rejected = false;
            try { calculate(routes(1,1,bad),bounds); } catch (const std::invalid_argument&) { rejected = true; }
            check(rejected,"Invalid reachable distance accepted");
        }
        for (const auto& b : {city::Boundary{{0,0},{0,1}},city::Boundary{{1,0},{0,1}},
                             city::Boundary{{0,0},{INFINITY,1}}}) {
            bool rejected = false;
            try { calculate({},b); } catch (const std::invalid_argument&) { rejected = true; }
            check(rejected,"Invalid boundary accepted");
        }
        for (std::uint32_t seed = 0; seed < 50; ++seed) {
            for (float roadDensity : {0.0f,0.1f,0.5f,1.0f}) {
                for (float buildingDensity : {0.1f,0.5f,1.0f}) {
                    const auto a = generated(seed,roadDensity,buildingDensity);
                    const auto b = generated(seed,roadDensity,buildingDensity);
                    close(a.finalScore,b.finalScore); close(a.distanceScore,b.distanceScore);
                    close(a.reachabilityScore,b.reachabilityScore);
                    close(a.averageShortestRoadDistance,b.averageShortestRoadDistance);
                    close(a.worstShortestRoadDistance,b.worstShortestRoadDistance);
                    check(a.reachableEmergencyLocations == b.reachableEmergencyLocations, "Nondeterministic counts");
                }
            }
        }
        std::cout << "Passed scoring fixtures and 50 seeds x 4 road densities x 3 building densities.\n"
                  << std::fixed << std::setprecision(2);
        for (std::uint32_t seed : {7u,42u,99u}) {
            const auto s = generated(seed,0.5f,0.5f);
            std::cout << "Seed " << seed << ", road/building density 0.5/0.5: reachable "
                      << s.reachableEmergencyLocations << '/' << s.totalEmergencyLocations
                      << ", average " << s.averageShortestRoadDistance << ", worst " << s.worstShortestRoadDistance
                      << ", distance score " << s.distanceScore << ", final " << s.finalScore << '\n';
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
