#include "CandidateApplication.h"
#include "RoadGraphBuilder.h"
#include "AccessibilityAnalyzer.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace city {
const CitySnapshot& CandidateLayout::view() const {
    return showAfter && optimization ? optimization->optimized : original;
}
void CandidateApplication::generateCandidates() {
    if (candidateCount < 1 || candidateCount > 5) { throw std::invalid_argument("Candidate count must be 1 to 5"); }
    status = "Generating";
    // Build a replacement set first, keeping the previous set intact on error.
    std::vector<CandidateLayout> next;
    try {
        for (int i = 0; i < candidateCount; ++i) {
            CandidateLayout candidate;
            candidate.parameters = setup;
            candidate.parameters.seed = setup.seed + static_cast<std::uint32_t>(i); // uint32 wrap is defined.
            auto& state = candidate.original;
            state.data = CityGenerator::generateCity(boundary,candidate.parameters);
            state.graph = RoadGraphBuilder::build(state.data.roads);
            state.facilities = placeFacilities(state.data,state.graph,candidate.parameters.seed,facilityParameters);
            state.routes = AccessibilityAnalyzer::analyze(state.graph,state.facilities);
            state.score = AccessibilityScorer::calculate(state.routes,boundary);
            next.push_back(std::move(candidate));
        }
    } catch (...) { status = "Generation failed"; throw; }
    candidates_ = std::move(next);
    selected_ = 0; // Select A, not necessarily the highest-scoring candidate.
    status = "Ready";
    std::ostringstream report;
    report << std::fixed << std::setprecision(2) << "\nCANDIDATE LAYOUTS\n";
    for (std::size_t i = 0; i < candidates_.size(); ++i) {
        const auto& c = candidates_[i]; const auto& s = c.original.score;
        report << "Candidate " << static_cast<char>('A'+i) << " | Seed " << c.parameters.seed
               << " | Emergency Accessibility Score " << s.finalScore << " | Reachable "
               << s.reachableEmergencyLocations << '/' << s.totalEmergencyLocations << " | Avg ";
        if (s.reachableEmergencyLocations) { report << s.averageShortestRoadDistance; } else { report << "N/A"; }
        if (i == highestIndex()) { report << " | Highest-Scoring Candidate"; }
        report << '\n';
    }
    std::cout << report.str();
}
void CandidateApplication::selectCandidate(std::size_t index) {
    if (index >= candidates_.size()) { throw std::out_of_range("Invalid candidate index"); }
    selected_ = index;
    status = "Candidate Selected";
}
const CandidateLayout& CandidateApplication::selected() const { return candidates_.at(selected_); }
std::size_t CandidateApplication::highestIndex() const { return highestScoringIndex(candidates_); }
std::size_t CandidateApplication::highestScoringIndex(const std::vector<CandidateLayout>& candidates) {
    if (candidates.empty()) { throw std::invalid_argument("No candidates to compare"); }
    constexpr double comparisonTolerance = 1e-6;
    double maximum = candidates.front().original.score.finalScore;
    for (const auto& c : candidates) { maximum = std::max(maximum,c.original.score.finalScore); }
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (maximum-candidates[i].original.score.finalScore <= comparisonTolerance) { return i; }
    }
    return 0;
}
void CandidateApplication::optimizeSelectedCandidate() {
    auto& candidate = candidates_.at(selected_);
    if (!candidate.optimization) {
        status = "Optimizing";
        try {
            candidate.optimization = CityOptimizer::optimize(candidate.original.data,boundary,
                candidate.original.facilities,candidate.parameters,optimizationParameters);
        } catch (...) { status = "Optimization failed"; throw; }
        const auto& result = *candidate.optimization;
        std::ostringstream report;
        report << std::fixed << std::setprecision(2) << "\nOPTIMIZATION RESULT | Candidate "
               << static_cast<char>('A'+selected_) << " | Seed " << candidate.parameters.seed
               << "\nEmergency Accessibility Score: " << result.original.score.finalScore << " -> "
               << result.optimized.score.finalScore << " | Improvement +"
               << result.optimized.score.finalScore-result.original.score.finalScore
               << "\nAccepted roads: " << result.modificationsAccepted << " | Attempts: " << result.iterationsAttempted << '\n';
        if (!result.modificationsAccepted) { report << "No improving modification found.\n"; }
        if (debug) for (std::size_t i = 0; i < result.history.size(); ++i) {
            const auto& step = result.history[i];
            report << "Attempt " << i+1 << " | " << step.candidateScore << " | "
                   << (step.accepted ? "ACCEPTED" : "REJECTED") << '\n';
        }
        std::cout << report.str();
    }
    candidate.showAfter = true; status = "Optimized";
}
void CandidateApplication::showBefore() { candidates_.at(selected_).showAfter = false; }
void CandidateApplication::showAfter() {
    auto& c = candidates_.at(selected_); if (c.optimization) { c.showAfter = true; }
}
void CandidateApplication::toggleBeforeAfter() { if (selected().showAfter) { showBefore(); } else { showAfter(); } }
void CandidateApplication::selectIncident(int offset) {
    auto& c = candidates_.at(selected_);
    const auto count = c.view().routes.size();
    if (count) {
        const int next = (static_cast<int>(c.selectedIncident) + offset) % static_cast<int>(count);
        c.selectedIncident = static_cast<std::size_t>(next < 0 ? next + static_cast<int>(count) : next);
    }
}
void CandidateApplication::adjustDensity(bool road, int step) {
    float& value = road ? setup.roadDensity : setup.buildingDensity;
    const int current = static_cast<int>(value*10+0.5f);
    value = std::clamp(current+step,road ? 0 : 1,10)/10.0f; // Pending only, like sliders.
}
} // namespace city
