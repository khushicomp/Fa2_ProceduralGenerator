#pragma once

#include "CityOptimizer.h"
#include <optional>
#include <string>

namespace city {
struct CandidateLayout {
    GenerationParameters parameters;
    CitySnapshot original;
    std::optional<OptimizationResult> optimization;
    bool showAfter = false;
    std::size_t selectedIncident = 0;
    const CitySnapshot& view() const;
};

// Application-only orchestration: no rendering, input library, or new algorithms.
class CandidateApplication {
public:
    GenerationParameters setup;
    FacilityParameters facilityParameters;
    OptimizationParameters optimizationParameters;
    int candidateCount = 3;
    bool graphVisible = false;
    bool debug = false;
    std::string status = "Ready";

    void generateCandidates();
    void selectCandidate(std::size_t index);
    void optimizeSelectedCandidate();
    void showBefore();
    void showAfter();
    void toggleBeforeAfter();
    void selectIncident(int offset);
    void setGraphVisible(bool visible) { graphVisible = visible; }
    void adjustDensity(bool road, int step);

    const std::vector<CandidateLayout>& candidates() const { return candidates_; }
    std::size_t selectedIndex() const { return selected_; }
    std::size_t highestIndex() const;
    const CandidateLayout& selected() const;
    const CitySnapshot& view() const { return selected().view(); }
    static std::size_t highestScoringIndex(const std::vector<CandidateLayout>& candidates);
private:
    std::vector<CandidateLayout> candidates_;
    std::size_t selected_ = 0;
};
} // namespace city
