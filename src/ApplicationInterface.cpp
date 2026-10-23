#include "ApplicationInterface.h"
#include "VisualAssets.h"
#include <imgui.h>
#include <algorithm>
#include <exception>
#include <filesystem>

namespace ui {
namespace {
constexpr ImVec4 accent{0.28f,0.70f,0.84f,1};
constexpr ImVec4 success{0.40f,0.78f,0.57f,1};
constexpr ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
void beginPanel(const char* name, Rect r, ImGuiWindowFlags extra = 0) {
    ImGui::SetNextWindowPos({r.x,r.y}); ImGui::SetNextWindowSize({r.width,r.height});
    ImGui::Begin(name,nullptr,panelFlags | extra);
}
void distance(const char* label, const city::AccessibilityScore& score, bool worst = false) {
    if (score.reachableEmergencyLocations) {
        ImGui::Text("%s: %.2f",label,worst ? score.worstShortestRoadDistance : score.averageShortestRoadDistance);
    } else { ImGui::Text("%s: N/A",label); }
}
}
void ApplicationInterface::remember(const std::string& name) {
    const auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
    targets[name] = {min.x,min.y,max.x-min.x,max.y-min.y};
}
void ApplicationInterface::initializeStyle() {
    ImGui::StyleColorsDark();
    auto& s = ImGui::GetStyle();
    s.WindowRounding = 0; s.ChildRounding = 6; s.FrameRounding = 5; s.GrabRounding = 6;
    s.WindowPadding = {14,12}; s.FramePadding = {8,5}; s.ItemSpacing = {8,6};
    s.WindowBorderSize = 1;
    s.Colors[ImGuiCol_WindowBg] = {0.045f,0.075f,0.11f,1};
    s.Colors[ImGuiCol_Border] = {0.22f,0.25f,0.29f,1};
    s.Colors[ImGuiCol_FrameBg] = {0.085f,0.13f,0.19f,1};
    s.Colors[ImGuiCol_Button] = {0.04f,0.32f,0.58f,1};
    s.Colors[ImGuiCol_ButtonHovered] = {0.06f,0.43f,0.74f,1};
    s.Colors[ImGuiCol_ButtonActive] = {0.08f,0.50f,0.82f,1};
    s.Colors[ImGuiCol_Header] = {0.17f,0.30f,0.37f,1};
    s.Colors[ImGuiCol_CheckMark] = accent; s.Colors[ImGuiCol_SliderGrab] = accent;
    s.Colors[ImGuiCol_Tab] = {0.09f,0.14f,0.20f,1};
    s.Colors[ImGuiCol_TabSelected] = {0.04f,0.29f,0.52f,1};
    s.Colors[ImGuiCol_TabHovered] = {0.08f,0.40f,0.65f,1};
    s.Colors[ImGuiCol_ChildBg] = {0.055f,0.085f,0.12f,1};
    // Use the installed Windows UI font without bundling a licensed font asset.
    // Other systems retain ImGui's built-in font at the same readable size.
    const char* font="C:/Windows/Fonts/segoeui.ttf";
    if (std::filesystem::exists(font)) { ImGui::GetIO().Fonts->AddFontFromFileTTF(font,16); }
    else { ImFontConfig config; config.SizePixels=16; ImGui::GetIO().Fonts->AddFontDefault(&config); }
    ImGui::GetIO().IniFilename = nullptr; // Fixed one-window layout; no stale ini state.
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}
void ApplicationInterface::applyPendingAction() {
    if (!pendingAction_) { return; }
    auto action = std::move(pendingAction_); pendingAction_ = {};
    try { action(); error.clear(); } catch (const std::exception& e) { error = e.what(); }
}
Rect ApplicationInterface::draw(city::CandidateApplication& app, float width, float height) {
    targets.clear();
    const float header = 70, footer = 38;
    const float sidebar = std::clamp(width*0.32f,350.0f,520.0f);
    const Rect cityArea{0,header,width-sidebar,height-header-footer};
    beginPanel("Control and analysis",{width-sidebar,header,sidebar,height-header-footer});
    // Commit UI actions before the next frame, so metrics and map always use
    // one complete snapshot instead of mixing old labels with new geometry.
    auto action = [&](std::function<void()> fn) { pendingAction_ = std::move(fn); };
    if (!error.empty()) { ImGui::TextColored({1,0.55f,0.4f,1},"Action failed"); ImGui::TextWrapped("%s",error.c_str()); }
    ImGui::TextColored(accent,"SELECTED CANDIDATE %c",static_cast<int>('A'+app.selectedIndex()));
    ImGui::TextWrapped("Emergency Accessibility Score");
    ImGui::SetWindowFontScale(1.45f);
    ImGui::Text("%.2f / 100",app.view().score.finalScore);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    if (ImGui::BeginTabBar("Workspace tabs")) {
        const bool setupTab = ImGui::BeginTabItem("Setup"); remember("tab.setup");
        if (setupTab) {
            ImGui::BeginChild("Setup content");
            ImGui::Spacing(); ImGui::TextColored(accent,"CITY SETUP");
            // Compact labeled rows leave room for comparing candidates together.
            if (ImGui::BeginTable("Parameters",2,ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label",0,0.43f); ImGui::TableSetupColumn("Value",0,0.57f);
                auto row=[](const char* label) { ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(label);
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1); };
                row("Base seed");
                ImGui::InputScalar("##Seed",ImGuiDataType_U32,&app.setup.seed); remember("seed");
                row("Road density");
                ImGui::SliderFloat("##Road density",&app.setup.roadDensity,0.0f,1.0f,"%.2f",ImGuiSliderFlags_AlwaysClamp); remember("density.road");
                row("Building density");
                ImGui::SliderFloat("##Building density",&app.setup.buildingDensity,0.1f,1.0f,"%.2f",ImGuiSliderFlags_AlwaysClamp); remember("density.building");
                row("Candidate count");
                ImGui::SliderInt("##Candidate count",&app.candidateCount,1,5,"%d",ImGuiSliderFlags_AlwaysClamp); remember("count");
                ImGui::EndTable();
            }
            if (ImGui::Button("Generate Candidates",{-1,0})) { action([&app] { app.generateCandidates(); }); }
            remember("generate");
            ImGui::TextDisabled("Edits apply on Generate (or R).");
            ImGui::Separator(); ImGui::TextColored(accent,"CANDIDATE LAYOUTS");
            ImGui::TextWrapped("Emergency Accessibility Score / 100");
            const int columns=ImGui::GetContentRegionAvail().x>=390 ? 3 : 2;
            if (ImGui::BeginTable("Candidate cards",columns,ImGuiTableFlags_SizingStretchSame)) {
            for (std::size_t i = 0; i < app.candidates().size(); ++i) {
                const auto& c = app.candidates()[i]; const auto& s = c.original.score;
                ImGui::TableNextColumn();
                ImGui::PushID(static_cast<int>(i));
                ImGui::PushStyleColor(ImGuiCol_Border,i==app.selectedIndex() ? accent : ImVec4{0.18f,0.24f,0.31f,1});
                ImGui::BeginChild("Card",{0,190},ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar);
                ImGui::SetWindowFontScale(0.9f);
                ImGui::Text("Candidate %c",static_cast<int>('A'+i));
                ImGui::Text("Seed: %u",c.parameters.seed);
                if (i == app.highestIndex()) { ImGui::TextColored(success,"Highest score"); }
                else { ImGui::TextDisabled(" "); }
                ImGui::Text("Score: %.2f",s.finalScore);
                ImGui::Text("Reachable: %zu / %zu",s.reachableEmergencyLocations,s.totalEmergencyLocations);
                distance("Avg",s);
                if (ImGui::Button(i == app.selectedIndex() ? "Selected" : "Select",{-1,0})) {
                    action([&app,i] { app.selectCandidate(i); });
                }
                remember("candidate."+std::to_string(i));
                ImGui::SetWindowFontScale(1.0f); ImGui::EndChild();
                ImGui::PopStyleColor(); ImGui::PopID();
            }
            ImGui::EndTable();
            }
            ImGui::TextWrapped("Highest applies only to this generated set, using original scores.");
            ImGui::EndChild(); ImGui::EndTabItem();
        }
        const bool analysisTab = ImGui::BeginTabItem("Analysis"); remember("tab.analysis");
        if (analysisTab) {
            ImGui::BeginChild("Analysis content");
            ImGui::Spacing(); ImGui::TextColored(accent,"EMERGENCY ANALYSIS");
            const auto& state = app.view(); const auto& score = state.score;
            ImGui::Text("Reachability: %zu / %zu",score.reachableEmergencyLocations,score.totalEmergencyLocations);
            ImGui::Text("Reachability Score: %.2f",score.reachabilityScore);
            ImGui::Text("Distance Score: %.2f",score.distanceScore);
            distance("Average road distance",score); distance("Worst road distance",score,true);
            ImGui::Separator(); ImGui::TextColored(accent,"SELECTED INCIDENT");
            if (state.routes.empty()) { ImGui::TextWrapped("No incidents: generate a layout with roads."); }
            else {
                const auto& route = state.routes[app.selected().selectedIncident];
                ImGui::Text("Emergency: E%d",route.emergencyId);
                if (route.reachable) {
                    ImGui::Text("Nearest hospital: H%d",route.hospitalId);
                    ImGui::Text("Road distance: %.2f",route.distance);
                } else { ImGui::TextColored({1,0.55f,0.4f,1},"UNREACHABLE"); ImGui::TextUnformatted("Nearest hospital / distance: N/A"); }
                ImGui::Text("Reachable hospitals: %zu",route.reachableHospitalIds.size());
            }
            ImGui::BeginDisabled(state.routes.empty());
            if (ImGui::Button("Previous")) { action([&app] { app.selectIncident(-1); }); } remember("incident.previous");
            ImGui::SameLine();
            if (ImGui::Button("Next")) { action([&app] { app.selectIncident(1); }); } remember("incident.next");
            ImGui::EndDisabled();
            ImGui::Separator(); ImGui::TextColored(accent,"VIEW OPTIONS");
            bool graph = app.graphVisible;
            if (ImGui::Checkbox("Show Road Graph",&graph)) { action([&app,graph] { app.setGraphVisible(graph); }); }
            remember("graph");
            bool detailed=visual::detailed();
            if (ImGui::Checkbox("Detailed City Visuals",&detailed)) { visual::setDetailed(detailed); }
            ImGui::Spacing();
            ImGui::TextWrapped("White cross: hospital\nRed diamond: incident\nYellow: selected road route\nCyan: accepted connector road");
            ImGui::TextDisabled("Gray access links are not scored.");
            ImGui::EndChild(); ImGui::EndTabItem();
        }
        const bool optimizeTab = ImGui::BeginTabItem("Optimize"); remember("tab.optimize");
        if (optimizeTab) {
            ImGui::BeginChild("Optimization content");
            ImGui::Spacing(); ImGui::TextColored(accent,"LAYOUT OPTIMIZATION");
            ImGui::TextWrapped("Original Emergency Accessibility Score");
            ImGui::Text("%.2f / 100",app.selected().original.score.finalScore);
            if (ImGui::Button("Optimize Selected Layout",{-1,0})) { action([&app] { app.optimizeSelectedCandidate(); }); }
            remember("optimize");
            const auto& c = app.selected();
            if (c.optimization) {
                const auto& result = *c.optimization;
                const auto& before = result.original.score; const auto& after = result.optimized.score;
                ImGui::Separator(); ImGui::TextWrapped("Emergency Accessibility Score");
                ImGui::Text("BEFORE %.2f  ->  AFTER %.2f",before.finalScore,after.finalScore);
                ImGui::TextColored(success,"IMPROVEMENT +%.2f",after.finalScore-before.finalScore);
                if (before.reachableEmergencyLocations && after.reachableEmergencyLocations) {
                    ImGui::Text("Avg distance: %.2f -> %.2f",before.averageShortestRoadDistance,after.averageShortestRoadDistance);
                } else { distance("Before avg distance",before); distance("After avg distance",after); }
                ImGui::Text("Reachable: %zu/%zu -> %zu/%zu",before.reachableEmergencyLocations,before.totalEmergencyLocations,
                            after.reachableEmergencyLocations,after.totalEmergencyLocations);
                ImGui::Text("Accepted connector roads: %d",result.modificationsAccepted);
                ImGui::Text("Iterations: %d",result.iterationsAttempted);
                ImGui::Text("Rejected modifications: %d",result.modificationsRejected);
                if (!result.modificationsAccepted) { ImGui::TextWrapped("No improving modification found."); }
            } else { ImGui::TextWrapped("Only roads change. Hospital and incident positions remain fixed."); }
            ImGui::Separator();
            if (ImGui::RadioButton("Before",!c.showAfter)) { action([&app] { app.showBefore(); }); } remember("before");
            ImGui::SameLine(); ImGui::BeginDisabled(!c.optimization);
            if (ImGui::RadioButton("After",c.showAfter)) { action([&app] { app.showAfter(); }); } remember("after");
            ImGui::EndDisabled();
            ImGui::TextWrapped("Greedy local search improves this objective; it does not guarantee a global optimum.");
            ImGui::EndChild(); ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    // Draw metadata after actions so every panel describes the current selection.
    beginPanel("Header",{0,0,width,header},ImGuiWindowFlags_NoScrollbar);
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextUnformatted("Procedural City Generator");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::TextDisabled("Emergency Accessibility & Layout Optimization");
    ImGui::SameLine(width-370);
    ImGui::Text("Candidate %c | Seed %u | %s",static_cast<int>('A'+app.selectedIndex()),
                app.selected().parameters.seed,app.selected().showAfter ? "AFTER" : "BEFORE");
    ImGui::End();
    beginPanel("Status",{0,height-footer,width,footer},ImGuiWindowFlags_NoScrollbar);
    const auto& c = app.selected(); const auto& state = app.view();
    if (c.showAfter && c.optimization) {
        ImGui::Text("AFTER | Candidate %c | Emergency Accessibility Score %.2f | Improvement +%.2f | Added roads %d",
            static_cast<int>('A'+app.selectedIndex()),state.score.finalScore,
            state.score.finalScore-c.original.score.finalScore,c.optimization->modificationsAccepted);
    } else {
        ImGui::Text("BEFORE | Candidate %c | Seed %u | Roads %zu | Buildings %zu | Graph %zu nodes / %zu edges",
            static_cast<int>('A'+app.selectedIndex()),c.parameters.seed,state.data.roads.size(),state.data.buildings.size(),
            state.graph.nodes.size(),state.graph.edges.size());
    }
    ImGui::End();
    return cityArea;
}
} // namespace ui
