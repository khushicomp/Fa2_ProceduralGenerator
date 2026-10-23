# Research paper verification report

**Verified 2026-10-18:** current local codebase through property-composition Stage 3. No application source or behavior was changed. SHA-256 comparisons found zero changes in 63 protected source/header/test/tool/asset/build-definition/README files. Measurements used a separate local driver linked to the application's existing algorithm objects.

**Manuscript unavailable:** no research-paper manuscript was found among project Markdown/PDF/DOCX/TeX files, and no manuscript path/text was supplied. Section 8 is a code-based claim checklist, not a review of specific paper sentences. Historical README descriptions are not treated as evidence of current behavior.

## 1. Implemented features and exact sources

| Feature | Source and functions |
| --- | --- |
| Seeded roads, blocks, buildings | [CityGenerator.cpp](../src/CityGenerator.cpp): `positions`, `CityGenerator::generate`, `generateCity`, `rebuildFromRoads`; data/constants in [City.h](../include/City.h) |
| Weighted undirected graph | [RoadGraphBuilder.cpp](../src/RoadGraphBuilder.cpp): `RoadGraphBuilder::build`; structures in [RoadGraph.h](../include/RoadGraph.h) |
| Hospital/incident placement | [Emergency.cpp](../src/Emergency.cpp): `placeFacilities` |
| BFS and shortest routes | [AccessibilityAnalyzer.cpp](../src/AccessibilityAnalyzer.cpp): `AccessibilityAnalyzer::analyze` |
| Accessibility metrics | [AccessibilityScorer.cpp](../src/AccessibilityScorer.cpp): `AccessibilityScorer::calculate` |
| Greedy optimization | [CityOptimizer.cpp](../src/CityOptimizer.cpp): `CityOptimizer::optimize`, local `candidates`, `evaluate`, `reattach` |
| Candidate comparison/snapshots | [CandidateApplication.cpp](../src/CandidateApplication.cpp): `generateCandidates`, `highestScoringIndex`, `selectCandidate`, `optimizeSelectedCandidate`, `CandidateLayout::view` |
| UI and lifecycle | [ApplicationInterface.cpp](../src/ApplicationInterface.cpp): `initializeStyle`, `draw`, `applyPendingAction`; [main.cpp](../src/main.cpp): `main`, `onKey` |
| City and analytical rendering | [Renderer.cpp](../src/Renderer.cpp): `resizeRegion`, `draw`, `drawStreets`, `streetStrip`, `drawGraph`, `drawEmergency` |
| Building sprites/textures | [VisualAssets.cpp](../src/VisualAssets.cpp): `initialize`, `layoutBuilding`, `drawBuilding`, `sprite`; [TextureManager.cpp](../src/TextureManager.cpp): `decode`, `TextureManager::load`, `clear` |
| Streets and properties | [StreetPresentation.h](../include/StreetPresentation.h): `streetProfiles`, `streetJunctions`; [VisualComposition.cpp](../src/VisualComposition.cpp): `composition`, `composeProperties`, `buildingPresentation`, `replay`, `crown` |
| GUI smoke validation | [SmokeValidation.cpp](../src/SmokeValidation.cpp): `keyboardStep`, `mouseStep`, `checkFrame` |

Simulation data is independent of OpenGL. Presentation envelopes, property surfaces, paths, planting and parking marks are decorative. Detailed buildings use assets first and procedural fallbacks. Streets have clearance-limited sidewalks, curbs, asphalt and dashed centers. Crosswalk bars have been removed.

## 2. Algorithms and scoring

**Generation.** `std::mt19937`, explicit shuffles and bounded jitter select axis-aligned full-boundary roads. Per axis, slot width is twice minimum spacing; count is `ceil(density * floor(axisLength / slotWidth))`. Sorted separators form cells, inset six units into blocks. Approximately forty-unit lots contain randomized rectangular buildings, minimum side twelve and spacing six; each block retains `ceil(buildingDensity * lotCount)` shuffled buildings. Buildings use a separately initialized random stream. Defaults: boundary `(100,100)-(900,700)`, spacing 40, road density [0,1], building density [0.1,1].

**Graph and facilities.** Endpoints/perpendicular crossings become exact-coordinate-deduplicated nodes; consecutive nodes along each road form unique undirected edges weighted by Euclidean length. Adjacency stores both directions. Default three hospitals/four incidents prefer shuffled building centers, then block centers, then graph sites. Nearest-node ties retain the lower node ID. Insufficient sites reduce counts; empty graphs produce no facilities. Access connectors are excluded from route cost.

**BFS/Dijkstra.** Per incident, queue-based BFS collects reachable hospital IDs. With none reachable, the result has infinite distance and an empty path. Otherwise min-priority-queue Dijkstra skips stale entries, relaxes edges and records predecessors. Minimum road distance selects the hospital; exact ties choose its lower ID. Reversing predecessors reconstructs the route. Same-node routes have zero distance. BFS is O(V+E); heap Dijkstra on this simple graph is O((V+E) log V), per incident, plus hospital processing. Graph construction also performs road-pair checks and road-by-node scans; it is not merely linear in graph size.

**Score.** For N incidents, K reachable, mean reachable shortest road distance d, and boundary diagonal L=1000 world units:

```text
R = 100 * K / N
D = 100 * (1 - min(d / L, 1))   if K > 0; otherwise D = 0
S = clamp(0.60 * R + 0.40 * D, 0, 100)
```

N=0 returns zero scores. Unreachable distances do not enter the mean/worst metrics. Worst distance is reported but not separately weighted. These distances are world-coordinate units, not seconds or calibrated meters. Weights are in `include/AccessibilityScorer.h`.

**Greedy optimizer.** Propose full-span midpoint roads in current gaps, requiring at least two perpendicular connections, spacing and fixed-facility clearance. Shuffle proposals deterministically; rebuild the graph, reattach the same physical facilities, run BFS/Dijkstra and score. Accept only improvement greater than epsilon; regenerate proposals after acceptance. Defaults in `include/CityOptimizer.h`: optimizer seed **42**, 100 attempts, epsilon 1e-6, spacing 40, facility clearance 10. Stop on exhausted proposals, budget or near-perfect score. Blocks/buildings are rebuilt after accepted roads. No deletion, relocation or global-optimum guarantee is implemented.

**Candidate comparison.** Generate 1-5 candidates (default three) with unsigned base seed+i, wrapping at 2^32. Every candidate uses the same pipeline. Highest original score wins; scores within 1e-6 of the maximum tie in favor of the lower index. A is selected initially regardless of ranking. Each candidate caches its own optimization, incident selection and view. Repeated Optimize reuses that result. This is finite comparison, not exhaustive search.

## 3. Actual Computer Graphics implementation

Stack: C++17, OpenGL **2.1 compatibility/fixed-function**, GLFW 3.4, bundled GLAD 0.1.36, Dear ImGui 1.91.9b and Windows WIC PNG decoding. Dependencies are defined in [CMakeLists.txt](../CMakeLists.txt).

| Technique | Implemented calls/behavior |
| --- | --- |
| Orthographic projection | `resizeRegion`: `glMatrixMode`, `glLoadIdentity`, `glOrtho` |
| Viewport/clipping | `glViewport`, framebuffer/DPI conversion, `glEnable(GL_SCISSOR_TEST)`, `glScissor` |
| Immediate-mode primitives | `glBegin/glEnd`, `glVertex2f/glVertex2fv`: quads for sprites/roads/surfaces, triangles for fallback roofs, triangle fans for canopies, lines/points for graph overlays |
| Translation/scaling | Local sprite/marker offsets added to world centers; aspect-preserving contain scaling. No explicit object `glTranslatef`/`glScalef` path |
| Quarter-turn sprite orientation | Swapped dimensions and cycled UV indices using `glTexCoord2fv`; not arbitrary-angle geometry rotation or `glRotatef` |
| Textures/blending | `glGenTextures`, `glBindTexture`, `glTexImage2D`, `glTexParameteri` with linear filtering/clamp-to-edge; `glEnable(GL_BLEND)`, `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` |
| Visual hierarchy | Ground/property paint, buildings, streets, optional dimmed graph, route/facilities, then ImGui; not a 3D depth-buffered scene |
| Detail/cache | Stable geometry-hash artwork choice, cached property paint and screen-scale parking marks; CPU clearance checks |

Nineteen PNGs load: six residential, four commercial, four office, one hospital and four trees. Current canopies are polygons: the loaded tree sprites are not called by scene rendering. The complete atlas and unextracted vehicle/landscape props are not rendered. Artwork occupies 81% of its fitted property rectangle, not 81% of the simulation footprint or every visible lawn surface.

**Not implemented:** custom DDA/Bresenham rasterization, explicit object transformation-matrix classes, arbitrary-angle editing, perspective/3D rendering, custom GLSL shading, traffic animation/signals or the proposed later dense vegetation stage. OpenGL rasterization is not evidence of a student-implemented line rasterizer.

## 4. Controls and UI

A resizable 1280x800 window (minimum 1000x700) contains the city viewport, candidate/seed/view header, sidebar and status bar. Setup offers seed/densities/count, Generate and candidate cards. Analysis shows score components, reachability, average/worst distance, incident/hospital details, Previous/Next, graph and Detailed City Visuals toggles. Optimize shows before/after metrics, accepted roads, attempts and view controls. There is no separate selected-route visibility checkbox; a route is drawn when available.

| Key | Action |
| --- | --- |
| R | Generate deterministic candidates from pending settings; no fresh random base seed |
| O / B | Optimize selected candidate / toggle Before-After |
| P / G | Next incident / graph toggle |
| Up-Down / Right-Left | Pending road/building density +/-0.1 |
| Escape | Exit |

ImGui keyboard capture suppresses shortcuts. Setup changes apply on Generate/R. Work is synchronous. CLI: `--demo`, `--seed N`, `--debug`, `--smoke-test`, `--help`; explicit seed overrides the demo seed. There is no layout persistence/export UI.

## 5. Automated test results

Fresh existing-workspace Debug build succeeded. **12/12 CTest tests passed, 0 failures, 16.15 seconds** on this audit run:

| Test | Result |
| --- | --- |
| CityGeneratorTests | Pass |
| CityBuildingTests | Pass |
| RoadGraphTests | Pass |
| AccessibilityTests | Pass |
| AccessibilityScoreTests | Pass |
| CityOptimizerTests | Pass |
| CandidateApplicationTests | Pass |
| GraphicsSmokeTest | Pass |
| SeedDemoSmokeTest | Pass |
| DemoSmokeTest | Pass |
| InvalidSeedTest | Pass: expected nonzero exit |
| VisualAssetTests | Pass |

VisualAssetTests is Windows-only in CMake. Graphics tests use hidden real OpenGL windows and synthetic input; they do not establish manual usability, reference-image matching or all-driver compatibility. The measurement driver below is not an additional CTest test. Evidence: `.tools/paper-build.log`, `.tools/paper-tests.log`, `build/Testing/Temporary/LastTest.log`.

## 6. Five-seed measured results

**Protocol:** predetermined seeds 42-46; road density **0.50**, building density **0.50**, spacing 40, boundary 800x600, default three hospitals/four incidents. Optimizer seed remains **42 for all cities**, with the defaults in section 2. Each seed had one warm-up and 21 fresh timed runs; no cached optimization was timed. Serialized geometry, graphs, facilities, routes, scores and optimization histories matched across repetitions; physical facility coordinates remained unchanged within each before/after pair.

| Seed | Reachable before -> after | Mean road distance before -> after | Score before -> after (/100) | Gain (points) | Added roads | Attempts |
| --- | --- | --- | --- | --- | --- | --- |
| 42 | 4/4 -> 4/4 | 306.71 -> 135.53 | 87.73 -> 94.58 | +6.85 | 4 | 17 |
| 43 | 4/4 -> 4/4 | 202.59 -> 202.59 | 91.90 -> 91.90 | +0.00 | 0 | 5 |
| 44 | 4/4 -> 4/4 | 363.47 -> 333.66 | 85.46 -> 86.65 | +1.19 | 3 | 13 |
| 45 | 4/4 -> 4/4 | 395.70 -> 288.45 | 84.17 -> 88.46 | +4.29 | 4 | 11 |
| 46 | 4/4 -> 4/4 | 199.73 -> 199.73 | 92.01 -> 92.01 | +0.00 | 0 | 6 |

Distances are world units and means include only reachable incidents. All five start with 9 roads, 38 nodes and 49 undirected edges. Seed 42 ends with 13 roads, 66 nodes and 93 edges; buildings change from 91 to 84 because optimization rebuilds subdivisions. Rendering does not mutate simulation buildings, but optimization can rebuild them.

**Interpretation:** three of five seeds improve; two do not. Reachability is already 100% throughout. These results support aggregate distance/score improvements in some seeds, not improved reachability, guaranteed improvement or real emergency-response-time reduction. Facility locations differ across seeds; comparisons are paired within seed rather than a common-demand cross-city experiment.

### Runtime measurements

Host: Windows, Intel Core i5-12450HX; GCC 14.1.0 / CMake 3.31.6. The driver links freshly built application **Debug algorithm objects** (`-g`, no optimization flag). Timings use `std::chrono::steady_clock`.

Generation includes geometry, graph, facilities, BFS/Dijkstra and initial scoring for one city. Optimization includes baseline reevaluation, attempted proposals and final subdivision rebuilding. Both exclude UI/rendering, textures, console output, serialization and process startup. Values are **median milliseconds (observed min-max)** over 21 runs, not confidence intervals or FPS.

| Seed | Generation + initial evaluation, ms | Optimization, ms |
| --- | --- | --- |
| 42 | 0.477 (0.365-0.626) | 7.664 (5.834-9.367) |
| 43 | 0.371 (0.333-0.600) | 1.246 (1.184-2.518) |
| 44 | 0.367 (0.321-0.561) | 3.699 (3.254-6.746) |
| 45 | 0.378 (0.347-0.609) | 3.886 (3.477-6.192) |
| 46 | 0.360 (0.317-0.605) | 1.387 (1.311-2.230) |

Local reproduction from the project root:

```powershell
& .\.tools\python\cmake\data\bin\cmake.exe --build build --parallel 4
& .\.tools\python\cmake\data\bin\ctest.exe --test-dir build --output-on-failure
$objects = @('CityGenerator','RoadGraphBuilder','Emergency','AccessibilityAnalyzer','AccessibilityScorer','CityOptimizer') |
    ForEach-Object { "build/CMakeFiles/ProceduralCityGenerator.dir/src/$_.cpp.obj" }
g++ -std=c++17 -g -Iinclude .tools/PaperMeasure.cpp $objects -o build/PaperMeasure.exe
.\build\PaperMeasure.exe
```

Raw results: `.tools/paper-measurements.csv`; supplemental driver: `.tools/PaperMeasure.cpp`. These ignored local audit artifacts must accompany any separate reproducibility package. For interactive reproduction, launch `ProceduralCityGenerator.exe --demo --seed 42` (substitute 43-46), select A and optimize. Timings depend on hardware/load/build flags; no Release or rendering benchmark was performed here.

## 7. System limitations

- Full-boundary axis-aligned roads and rectangular subdivision; no GIS, terrain, irregular street-generation model, deletion or intersection relocation. Graph coordinate identity is exact.
- Edge costs are geometric distance only: no congestion, speeds, one-way restrictions, turn penalties, hospital capacity or service times. Facility-to-node access costs are omitted.
- Nearest-node reassociation can contribute to improvements. Fixed facilities can lie outside rebuilt buildings after optimization.
- Fixed heuristic score weights lack real-world calibration; no construction-cost/robustness objective. A higher aggregate score need not improve every incident route.
- Only 1-5 candidates and bounded greedy search; synchronous execution and repeated graph rebuilding limit scale.
- Five seeds at one density pair do not establish statistical significance, general scalability or superiority over another optimizer. No external dataset or comparative baseline was evaluated.
- Legacy OpenGL desktop requirement; WIC decoding is Windows-specific, with procedural fallbacks elsewhere. Decorative land-use categories are not simulation zoning data.

## 8. Unsupported paper claims to check

The manuscript was unavailable. These are conditional checks, not quotations attributed to the paper:

| Claim, if present | Supported correction |
| --- | --- |
| Custom DDA/Bresenham, arbitrary object rotation, explicit translation matrices, shader-based 3D | Use the actual fixed-function 2D techniques in section 3; coordinate-offset translation is implemented |
| Genetic algorithm, simulated annealing, A*, globally optimal city or exhaustive search | Finite candidate comparison and deterministic greedy add-only search |
| Every seed/every incident improves | Seeds 43 and 46 accept no change; aggregate score alone controls acceptance |
| Reachability improved in this experiment | All five cases remain 4/4 reachable |
| Measured real response time or validated emergency-service performance | World-coordinate shortest road distance and an educational weighted score |
| Buildings/graph attachments never change | Visual rendering preserves simulation data; optimization rebuilds buildings/graph and reattaches fixed facility positions |
| All 19 assets or the whole atlas appear in the scene | Buildings use sprites; current trees use polygons; unextracted props are absent |
| Crosswalks at intersections or vector-first detailed buildings | Crosswalks were removed; buildings are asset-first |
| Proven photorealistic/reference-matching quality | No perceptual study or quantitative visual-quality experiment was performed |
| General real-time/FPS, Release performance or scalability conclusion | Only the stated single-machine Debug CPU timings were measured |

Repository statements also requiring care: historical README sections describe eleven tests, 0.04 asset matching tolerance, original-footprint-only containment, vector-first buildings and zebra crossings. Current behavior is twelve Windows tests, 0.10 tolerance, enlarged presentation envelopes, asset-first buildings and no crosswalk bars. Historical renderer timings are not interchangeable with this report's algorithm timings.

**Defensible paper summary:** the prototype combines seeded rectangular city generation, explicit BFS/Dijkstra, a normalized accessibility score, finite candidate comparison and greedy road additions, visualized through a 2D OpenGL/ImGui interface. The five-seed experiment shows aggregate score improvement in three cases while preserving physical facility positions, under the stated simplified distance model.
