#include "CandidateApplication.h"
#include "AccessibilityAnalyzer.h"
#include "RoadGraphBuilder.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool ok,const char* message) { if(!ok) { throw std::runtime_error(message); } }
void close(double a,double b) { check(std::abs(a-b)<1e-9,"Values differ"); }
bool samePoint(city::Point a,city::Point b) { return a.x==b.x && a.y==b.y; }
void same(const city::CitySnapshot& a,const city::CitySnapshot& b) {
    check(a.data.roads.size()==b.data.roads.size() && a.data.blocks.size()==b.data.blocks.size() &&
          a.data.buildings.size()==b.data.buildings.size(),"Geometry counts differ");
    for(std::size_t i=0;i<a.data.roads.size();++i) {
        check(samePoint(a.data.roads[i].start,b.data.roads[i].start) && samePoint(a.data.roads[i].end,b.data.roads[i].end),"Roads differ");
    }
    auto rectangles=[](const auto& x,const auto& y) {
        for(std::size_t i=0;i<x.size();++i) { check(samePoint(x[i].min,y[i].min) && samePoint(x[i].max,y[i].max),"Rectangles differ"); }
    };
    rectangles(a.data.blocks,b.data.blocks); rectangles(a.data.buildings,b.data.buildings);
    check(a.graph.nodes.size()==b.graph.nodes.size() && a.graph.edges.size()==b.graph.edges.size(),"Graph sizes differ");
    for(std::size_t i=0;i<a.graph.nodes.size();++i) { check(samePoint(a.graph.nodes[i].position,b.graph.nodes[i].position),"Graph nodes differ"); }
    for(std::size_t i=0;i<a.graph.edges.size();++i) {
        const auto& x=a.graph.edges[i]; const auto& y=b.graph.edges[i];
        check(x.from==y.from && x.to==y.to && x.weight==y.weight,"Graph edges differ");
    }
    auto facilities=[](const auto& x,const auto& y) {
        check(x.size()==y.size(),"Facility counts differ");
        for(std::size_t i=0;i<x.size();++i) {
            check(x[i].id==y[i].id && x[i].associatedGraphNodeId==y[i].associatedGraphNodeId &&
                  samePoint(x[i].position,y[i].position),"Facilities differ");
        }
    };
    facilities(a.facilities.hospitals,b.facilities.hospitals); facilities(a.facilities.emergencies,b.facilities.emergencies);
    check(a.routes.size()==b.routes.size(),"Route count differs");
    for(std::size_t i=0;i<a.routes.size();++i) {
        const auto& x=a.routes[i]; const auto& y=b.routes[i];
        check(x.emergencyId==y.emergencyId && x.hospitalId==y.hospitalId && x.reachable==y.reachable &&
              x.distance==y.distance && x.pathNodeIds==y.pathNodeIds && x.reachableHospitalIds==y.reachableHospitalIds,"Routes differ");
    }
    close(a.score.finalScore,b.score.finalScore); close(a.score.averageShortestRoadDistance,b.score.averageShortestRoadDistance);
}
}
int main() {
    try {
        for(int count=1;count<=5;++count) {
            city::CandidateApplication app; app.candidateCount=count; app.setup.seed=42;
            app.generateCandidates();
            check(app.candidates().size()==static_cast<std::size_t>(count),"Requested count not produced");
            const auto initial=app.candidates();
            app.generateCandidates();
            for(std::size_t i=0;i<initial.size();++i) {
                const auto& c=app.candidates()[i];
                check(c.parameters.seed==42+i,"Wrong seed derivation");
                same(initial[i].original,c.original);
                // Re-evaluate through the unchanged pipeline, independently of orchestration.
                city::CitySnapshot independently;
                independently.data=city::CityGenerator::generateCity(city::boundary,c.parameters);
                independently.graph=city::RoadGraphBuilder::build(independently.data.roads);
                independently.facilities=city::placeFacilities(independently.data,independently.graph,c.parameters.seed);
                independently.routes=city::AccessibilityAnalyzer::analyze(independently.graph,independently.facilities);
                independently.score=city::AccessibilityScorer::calculate(independently.routes,city::boundary);
                same(c.original,independently);
                check(app.candidates()[app.highestIndex()].original.score.finalScore+1e-6>=c.original.score.finalScore,"Wrong highest score");
            }
        }
        std::vector<city::CandidateLayout> tied(3);
        tied[0].original.score.finalScore=80; tied[1].original.score.finalScore=80+0.5e-6; tied[2].original.score.finalScore=60;
        check(city::CandidateApplication::highestScoringIndex(tied)==0,"Tie not lower index");
        tied[2].original.score.finalScore=90;
        check(city::CandidateApplication::highestScoringIndex(tied)==2,"Highest index incorrect");
        city::CandidateApplication app; app.generateCandidates();
        const auto saved=app.candidates();
        check(app.selectedIndex()==0 && !app.selected().optimization,"Startup optimized/forced highest");
        app.showAfter(); check(!app.selected().showAfter,"After without optimization");
        app.selectCandidate(1); same(app.view(),saved[1].original);
        app.optimizeSelectedCandidate();
        check(app.selected().showAfter && app.selected().optimization,"Selected B not optimized");
        check(!app.candidates()[0].optimization && !app.candidates()[2].optimization,"Other candidate optimized");
        same(app.candidates()[0].original,saved[0].original); same(app.candidates()[2].original,saved[2].original);
        same(app.candidates()[1].original,saved[1].original);
        const auto optimized=app.view();
        app.toggleBeforeAfter(); same(app.view(),saved[1].original);
        app.showAfter(); same(app.view(),optimized);
        app.selectCandidate(0); same(app.view(),saved[0].original);
        check(std::abs(app.view().score.finalScore-87.731624)<0.001,"Seed 42 before changed");
        app.optimizeSelectedCandidate();
        check(std::abs(app.view().score.finalScore-94.5787)<0.001,"Seed 42 after changed");
        check(std::abs(app.view().score.averageShortestRoadDistance-135.533)<0.01 &&
              app.selected().optimization->modificationsAccepted==4,"Seed 42 demo metrics changed");
        app.selectIncident(1); check(app.selected().selectedIncident==1,"Next action");
        app.selectIncident(-1); check(app.selected().selectedIncident==0,"Previous action");
        app.setGraphVisible(true); check(app.graphVisible,"Graph action");
        const auto beforeEdits=app.candidates();
        app.setup.seed=99; app.adjustDensity(true,1); app.adjustDensity(false,-1); app.candidateCount=2;
        same(app.candidates()[0].original,beforeEdits[0].original);
        app.generateCandidates();
        check(app.candidates().size()==2 && app.selected().parameters.seed==99 && !app.selected().optimization,"Pending generation reset");
        app.setup.seed=std::numeric_limits<std::uint32_t>::max(); app.candidateCount=3; app.generateCandidates();
        check(app.candidates()[1].parameters.seed==0 && app.candidates()[2].parameters.seed==1,"Seed wrap policy");
        app.setup.roadDensity=0; app.generateCandidates();
        check(app.view().routes.empty() && app.view().score.finalScore==0,"Empty city handling");
        app.selectIncident(1); app.optimizeSelectedCandidate(); check(app.view().score.finalScore==0,"Empty optimization");
        app.candidateCount=6; bool rejected=false;
        try { app.generateCandidates(); } catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"Invalid count accepted");
        app={}; app.generateCandidates();
        std::cout<<"Candidate tests passed: counts, determinism, full pipeline consistency, ties, selection, isolation, pending edits, and seed-42 regression.\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
