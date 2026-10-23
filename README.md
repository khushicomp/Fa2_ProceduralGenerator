# Procedural City Generator with Layout Optimization

## 1. Project Overview

A feature-complete educational C++17 desktop application for procedural 2D cities,
emergency road-network analysis, candidate comparison, and greedy layout optimization.
The interface uses Dear ImGui with the existing GLFW/OpenGL renderer.

This is a **simplified educational 2D simulation**, not a real-world urban-planning
decision system. Phase 7 adds application orchestration and UI; the validated
city, graph, facility, routing, scoring, and optimization algorithms are unchanged.

## 2. Objective

Explore how procedural geometry and graph algorithms support an explicit emergency
accessibility objective. Compare a small generated candidate set, select any layout,
and improve its roads while keeping that layout's hospital/incident positions fixed.

## 3. Application Workflow

```text
Configure pending seed/densities/count
 -> Generate Candidates
 -> Compare original Emergency Accessibility Scores
 -> Select any candidate
 -> Inspect incidents/routes/graph
 -> Optimize Selected Layout
 -> Compare Before / After
 -> Inspect final result
```

Startup generates three candidates and selects A. Optimization never runs automatically.
Setup changes are pending until Generate Candidates or R; dragging sliders does not
replace the displayed city. Regeneration replaces the candidate set and clears its
optimization results. Switching candidates preserves each one's optimization and view.

## 4. Procedural Generation

Seeded slot selection and bounded jitter create full-boundary horizontal/vertical
roads with minimum parallel spacing. Road density is [0,1]; minimum spacing defaults
to 40 world units. The fixed city boundary is (100,100) to (900,700), within a
1000x800 Cartesian world. Width/height are deliberately not editable in this UI.

Sorted road/boundary coordinates delimit blocks. Cells are inset 6 units; tiny
buildable cells are discarded. Approximately 40-unit lots contain varied rectangular
buildings at least 12 units wide/high, with 6-unit spacing. Building density is
[0.1,1]. Counts change in discrete steps. The geometry remains ordinary vectors of
Road, Block, and Building, independent of OpenGL and ImGui.

## 5. Road Graph

RoadGraphBuilder creates nodes at every endpoint and horizontal/vertical crossing,
then connects consecutive nodes along each road. Edges have Euclidean segment-length
weights and adjacency entries in both directions. Duplicate/reversed roads do not
duplicate edges. The city boundary outline is not itself a graph road.

Intersections copy existing x/y coordinates without interpolation; exact finite
coordinate keys safely deduplicate this generated geometry. Nearly equal imported
coordinates remain distinct. Diagonal/nonfinite/zero-length roads are rejected.

## 6. Emergency Facilities

Default counts are three hospitals and four incidents, configured in FacilityParameters.
Seeded shuffled building centers are preferred, then block centers and unused graph
positions. Distinct visual locations attach to the nearest graph node by Euclidean
distance, with lower node id breaking ties. Insufficient sites reduce counts; an
empty road graph produces no facilities.

Facility-to-node access connectors are illustrative and excluded from route distance.
Facilities may share a node. Across independently generated candidates, facilities
can differ; during optimization of one candidate, all physical positions stay fixed.
Only nearest-node attachments may change.

## 7. BFS Reachability

AccessibilityAnalyzer explicitly runs BFS over adjacency from each emergency's node
and records all reachable hospital ids. Disconnected graphs are supported. This
reachability computation is independent of edge weights and separate from Dijkstra.

## 8. Dijkstra Routing

A min-priority queue computes shortest weighted road-network distances. The reachable
hospital with minimum road distance wins; equal distances choose the lower hospital id.
Predecessors are followed backward and reversed to reconstruct ordered path node ids.
The yellow route draws those actual graph edges, not a facility-to-hospital straight line.

Unreachable incidents have no selected hospital, infinite distance, and an empty path.
Facilities attached to the same node have a zero-distance, one-node route.

## 9. Emergency Accessibility Score

```text
ReachabilityScore = 100 * reachableEmergencies / totalEmergencies
referenceDistance = hypot(boundaryWidth, boundaryHeight)
DistanceScore = 100 * (1 - min(avgShortestRoadDistance / referenceDistance, 1))
FinalScore = 0.60 * ReachabilityScore + 0.40 * DistanceScore
```

Only reachable Dijkstra distances enter mean/worst metrics. The boundary diagonal
is 1000 world units, unaffected by window resizing. Connector distances remain
excluded. Worst distance is reported but not separately weighted. Reachability
receives the larger weight because connection to a hospital comes first.

Distance/final scores are clamped to [0,100]. No incidents or none reachable yields
zero scores and N/A distance displays. AccessibilityScorer is the sole implementation;
the GUI displays its stored values and never computes an alternative score.

## 10. Candidate Comparison

CandidateApplication is an OpenGL/ImGui-independent orchestration layer.
CandidateLayout stores generation parameters, an original CitySnapshot, an optional
OptimizationResult, its Before/After state, and its selected incident. A snapshot
contains geometry, graph, facilities, route results, and the existing score structure.

Generate N candidates (1-5, default 3), with seeds:

```text
Candidate A = base seed
Candidate B = base seed + 1
Candidate C = base seed + 2
...
```

Seeds use unsigned 32-bit arithmetic and wrap at 2^32. Each candidate runs through
CityGenerator -> RoadGraphBuilder -> placeFacilities -> AccessibilityAnalyzer ->
AccessibilityScorer. No second evaluation pipeline or objective is introduced.

The **Highest-Scoring Candidate** is the candidate with the highest original Emergency
Accessibility Score within this finite generated set. Scores within 1e-6 of the
maximum tie, with the lower candidate index winning. Original scores remain the
comparison baseline even after a candidate is optimized. A is selected initially,
not automatically the highlighted candidate. Any candidate can be selected.

Candidate generation does not enumerate every possible city or identify a globally
optimal city. Optimizing B cannot mutate A or C.

Actual --demo candidates (default densities 0.5/0.5):

| Candidate | Seed | Emergency Accessibility Score | Reachable | Average road distance |
| --- | --- | --- | --- | --- |
| A | 42 | 87.73 | 4/4 | 306.71 |
| B | 43 | 91.90 | 4/4 | 202.59 |
| C | 44 | 85.46 | 4/4 | 363.47 |

B is highlighted; the presenter remains free to choose A for the established demo.

## 11. Greedy Layout Optimization

The unchanged CityOptimizer proposes full-span midpoint connector roads in current
gaps, requiring perpendicular connections, minimum spacing, and fixed-facility clearance.
Every candidate rebuilds the graph, reattaches the same facilities, runs BFS/Dijkstra,
and is evaluated by AccessibilityScorer.

Accept only when candidateScore > currentScore + epsilon. Defaults: 100 attempts,
epsilon 1e-6, spacing 40, facility clearance 10, optimization seed 42. Search stops
at the budget, a near-perfect score, or exhausted candidates. Final buildings/blocks
are rebuilt around accepted roads without moving facilities. No improvement returns
the original geometry unchanged. The returned final score never decreases.

Greedy local search does **not** guarantee a global optimum, construction efficiency,
or shorter routes for every individual incident. Repeated Optimize uses that
candidate's cached result. Candidate switching retains all complete snapshots.

The unchanged seed-42 result is:

| Metric | Before | After |
| --- | --- | --- |
| Emergency Accessibility Score | 87.73 | 94.58 |
| Reachability | 4/4 | 4/4 |
| Average road distance | 306.71 | 135.53 |

Improvement: +6.85; accepted connector roads: 4; attempts: 17.

## 12. Application Interface

One 1280x800 window, minimum 1000x700, titled Procedural City Generator. A compact
header shows selected candidate/seed/Before-After view. The city occupies approximately 65-73%
of the width with a letterboxed, scissored OpenGL viewport; framebuffer scaling
accounts for high-DPI displays. It never draws beneath UI panels.

The sidebar uses three tabs to keep controls readable, with scrollable tab contents:

- **Setup:** base seed, pending density sliders, count, Generate Candidates, candidate
  responsive two/three-column cards, highest-score marker, and selection buttons.
- **Analysis:** stored Emergency Accessibility Score components, reachability, mean/worst
  distance, selected incident/hospital, reachable hospitals, Previous/Next, graph checkbox.
- **Optimize:** original score, Optimize Selected Layout, Before/After values, improvement,
  distances, reachability, accepted roads, attempts, and visible Before/After controls.

A prominent current-view Emergency Accessibility Score stays above the tabs.
The bottom status bar identifies the actual displayed snapshot and geometry/graph
counts, or optimized metrics. UI actions are committed before the next frame so
panels and the map describe the same complete state. Work is synchronous; no threads.

| Map item | Appearance |
| --- | --- |
| Boundary / existing roads | Subtle frame / dark asphalt, lighter borders, dashed center markings |
| Ground / landscaping | Muted block variation, sparse polygon trees and small park patches |
| Sidewalks | Thin neutral strips within the visual road setback |
| Buildings | Coordinate-stable slate, stone, and terracotta roofs; shallow roof planes or inset service units |
| Hospital | White cross on green square |
| Incident | Red diamond |
| Selected route / facility | Gold road stroke / gold backing square |
| Accepted roads in After | Restrained teal road borders around the same asphalt surface |
| Graph overlay | Fine blue-gray edges and small pale nodes |
| Access connection | Subtle gray line; excluded from scoring |

Environmental decorations are renderer-only, coordinate-deterministic, and capped at
24 trees and four small park patches. Read-only clearance checks avoid buildings,
roads, and facility markers. They never enter simulation data or graph calculations.

Dear ImGui **v1.91.9b** is fetched by CMake from its official source archive with a
SHA-256 hash. Only core files and official imgui_impl_glfw / imgui_impl_opengl2 backends
are compiled. The OpenGL2 backend matches the existing OpenGL 2.1 renderer. Uses installed Segoe UI on Windows with a built-in font fallback; no bundled
font assets or unrelated dependency. ImGui callbacks chain the
application callback; keyboard capture prevents typing in fields from firing shortcuts.

Source responsibilities:

- CandidateApplication: shared actions, candidate storage, selection, comparison.
- ApplicationInterface: ImGui layout and controls, no scoring/optimization algorithms.
- main.cpp: GLFW/GLAD/ImGui lifecycle, input, framebuffer viewport, drawing.
- SmokeValidation: automated keyboard/mouse interface checks.
- Existing algorithm modules and tests: unchanged.

## 13. Controls

| GUI action | Shortcut |
| --- | --- |
| Generate Candidates using pending parameters | R |
| Optimize Selected Layout | O |
| Toggle Before / After | B |
| Next incident | P |
| Show Road Graph | G |
| Pending road density +/-0.1 | Up / Down |
| Pending building density +/-0.1 | Right / Left |
| Exit | Escape |

Previous is also a visible incident button. All GUI/keyboard actions use the same
CandidateApplication state. Shortcuts are suppressed while ImGui captures keyboard
input. Click the city viewport to leave field/navigation focus when needed. Unlike
earlier single-city phases, R generates the deterministic pending candidate set;
it does not choose a nondeterministic base seed. Enter another seed to explore new cities.

## 14. Build Instructions

Windows 11 prerequisites: CMake 3.20+ on PATH, a C++17 compiler, and a driver supporting
OpenGL 2.1. VS Code is the editor; C/C++ and CMake Tools extensions are optional.
GLFW 3.4 and Dear ImGui v1.91.9b download/build automatically on first configure,
requiring access to GitHub. Generated GLAD 0.1.36 is included; Python is not required.

**MSVC:** install Visual Studio Build Tools 2022, Desktop development with C++, MSVC,
and Windows SDK. In PowerShell:

```powershell
cd F:\Myprojects\ProceduralCityGenerator
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Debug --parallel
.\build-msvc\Debug\ProceduralCityGenerator.exe --demo
```

**MinGW:** in the MSYS2 UCRT64 terminal:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make
```

Then in PowerShell, with Windows CMake on PATH:

```powershell
cd F:\Myprojects\ProceduralCityGenerator
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build-mingw --parallel
.\build-mingw\ProceduralCityGenerator.exe --demo
```

Keep the UCRT64 bin directory on PATH for runtime DLLs. Use separate build directories
per compiler. Adjust the project path if needed; for MSVC Release replace Debug with Release.
The current workspace also has local validation tools and an existing build:

```powershell
& .\.tools\python\cmake\data\bin\cmake.exe --build build --parallel 4
.\build\ProceduralCityGenerator.exe --demo
```

For offline source reuse, configure with
`-DFETCHCONTENT_SOURCE_DIR_GLFW=C:/path/to/glfw-3.4` and
`-DFETCHCONTENT_SOURCE_DIR_IMGUI=C:/path/to/imgui-1.91.9b`.
`build*/` and `.tools/` contain ignored validation artifacts. GLAD's license is included;
GLFW and ImGui licenses accompany their fetched sources. No manual ImGui installation.

## 15. Demo Instructions

```powershell
.\build\ProceduralCityGenerator.exe --demo
# Equivalent default-density base seed:
.\build\ProceduralCityGenerator.exe --seed 42
```

1. Compare A, B, and C in Setup (scroll within the tab if needed). B is highlighted.
2. Select B or C to demonstrate that selection is independent of the highest-score label.
3. Select A (seed 42). Open Analysis; inspect incidents with Previous/Next and graph checkbox.
4. Open Optimize. Note original Emergency Accessibility Score 87.73.
5. Press Optimize Selected Layout (or O after leaving UI keyboard focus).
6. Inspect 94.58, +6.85, average distance 135.53, four accepted roads.
7. Use Before/After controls or B. Facility positions stay fixed; roads/buildings/routes
   and metrics switch together. Switch candidates and return to show retained results.
8. Edit seed/densities/count and press Generate Candidates to create a fresh set.

No automatic optimization occurs. --demo sets base seed 42 and densities 0.5/0.5;
--seed overrides its seed regardless of argument order. Without options, configured
defaults are retained. Seeds are unsigned 32-bit integers. --debug prints optimization
attempts; --help lists options; invalid arguments exit with an error.

## 16. Testing

```powershell
ctest --test-dir build-mingw --output-on-failure
# MSVC:
ctest --test-dir build-msvc -C Debug --output-on-failure
# Current workspace:
& .\.tools\python\cmake\data\bin\ctest.exe --test-dir build --output-on-failure
.\build\ProceduralCityGenerator.exe --demo --smoke-test
```

All previous tests remain: city generation, buildings, graph, accessibility, scoring,
optimizer, graphics smoke, seed-demo smoke, demo smoke, and invalid-seed handling.
CandidateApplicationTests adds count, complete snapshot determinism, seed derivation/
wrap, independent pipeline evaluation, highest-score/ties, selection, optimization
isolation, pending setup edits, empty cities, and seed-42 regression. Algorithmic and
candidate test executables do not link ImGui or OpenGL.

The real OpenGL/ImGui smoke test retains graph/route/optimization/Before-After keyboard
checks and adds mouse events into actual widgets: tabs, graph checkbox, Previous/Next,
candidate selection, Optimize, Before/After, seed text entry, both density sliders,
count, and Generate. It verifies shared state and shortcut suppression while editing.
It also checks aspect ratio and viewport bounds at the requested presentation sizes
1280x800, 1366x768, 1600x900, and 1920x1080, plus 1000x700 and earlier aspect ratios.

The test requires a graphics-capable desktop session even though the window is hidden.
Validation uses MinGW GCC 14.1.0, CMake 3.31.6, and NVIDIA OpenGL 4.6. Framebuffer captures
are used for visual inspection. OS-level physical mouse/keyboard interaction and MSVC
are separate local checks; synthetic ImGui events do not claim to replace them.

Final Phase 7 validation: all 11 CTest tests passed. Normal and --demo launches
succeeded. Framebuffer inspection covered all four presentation sizes and 1000x700,
including consistent After metrics and rendering. SHA-256 comparisons confirmed
the six existing generation, graph, facility, analysis, scoring, and optimizer
implementation files were unchanged.

## 17. Limitations

- Educational axis-aligned, full-boundary roads and rectangular subdivisions; no GIS.
- Only 1-5 candidates from deterministic seeds; not an exhaustive search of possible cities.
- Highest-Scoring Candidate means highest original Emergency Accessibility Score in this set.
- Greedy add-only search has a finite budget and no global optimality or construction-cost guarantee.
- Fixed-facility nearest-node reassociation can contribute to improvements; access distances
  are excluded. Rebuilt buildings need not contain original facility sites.
- Individual incident routes need not all improve. Empty roads yield no facilities and score zero.
- Exact coordinate comparison targets generated geometry rather than noisy imported maps.
- Legacy OpenGL support is required. Sidebar tab contents scroll on smaller windows.
- Generation/optimization are synchronous and intended for small classroom examples.
- No manual drawing, traffic, 3D, accounts, databases, cloud services, or additional objectives.

Phase 7 completes the final application scope.


## Visual Asset Layer

Phase 7.2 adds a visual skin to the existing simulation. Assets do **not** affect
procedural generation, road/building/block coordinates, graph construction,
facility placement, BFS/Dijkstra routing, scoring, candidate comparison, or
optimization. Simulation data remains the only source of truth.

The supplied atlas is retained at `assets/source/city-atlas.png`. Nineteen tight
PNG crops live under `assets/buildings/{residential,commercial,office,hospital}`
and `assets/vegetation/trees`. See [asset inventory](assets/README.md) for every
filename and preparation details. Reproduce crops using:

```powershell
powershell -ExecutionPolicy Bypass -File tools/prepare-assets.ps1
```

Building classification is render-only: area below 450 world units squared uses
six residential variants; 450 to below 850 uses four commercial variants; 850+
uses four office variants. Measurements for seeds 42/43/44 at demo densities:
side lengths 12.00-66.41; areas 167.17-2223.89. Seed 42 has 23/44/24 buildings in
these categories. Building objects have no new type fields.

A stable FNV-style hash of the footprint's coordinate/dimension float bits selects
among textures within 0.04 log-aspect error of the closest match, allowing a
quarter-turn rotation. Contain scaling fits the entire sprite inside its original
footprint without stretching. Hospitals use the hospital skin only when their
existing position lies inside a displayed building; otherwise the exact-position
medical marker remains. Optimized snapshots can leave fixed facilities outside
rebuilt buildings, as before.

`TextureManager` decodes PNG into RGBA through Windows Imaging Component (WIC),
caches successes and failures, uploads once, and frees textures before context
shutdown. `VisualAssets` chooses sprites and draws alpha-blended OpenGL 2.1 quads.
There is no disk loading per frame. CMake copies `assets/` beside the executable
on build; lookup is relative to the executable, not the working directory.
Keep that folder with the executable when presenting/copying it.

No third-party dependency was added. Windows system libraries `windowscodecs`
and `ole32` provide PNG decoding; they are included with Windows. The supplied
implementation uses procedural fallbacks on non-Windows platforms.

Analysis > View Options > **Detailed City Visuals** defaults on. Turning it off
restores procedural roofs/canopies. A missing/corrupt building sprite uses another
available compatible sprite or the procedural roof; missing tree sprites are
skipped; hospital markers always remain. Sparse tree/park placement retains the
existing deterministic clearance checks and limits (24 trees, four patches).
Roads, sidewalks, routes, and graph overlays retain their original geometry.

All original tests are unchanged. `VisualAssetTests` additionally checks PNG alpha,
cache reuse, missing/corrupt file fallback, GPU cleanup, sprite containment,
repeated-frame determinism, and the detailed-visual toggle. Both test suites need
a graphics-capable desktop. Asset preparation is complete for all enabled sprites;
unused atlas props have intentionally not been added.

Phase 7.2 is the final visualization phase. The project is frozen for bug fixes.


Phase 7.2 validation: all 12 CTest tests pass (the original 11 are unchanged).
All 19 textures loaded successfully. A separate copy of the executable with no
asset directory passed the complete demo smoke test using fallbacks. Normal
--demo launch succeeded. Framebuffer inspection covered 1280x800, 1366x768,
1600x900, and 1920x1080, including the optimized/graph view. Seed 42 remains
87.73 -> 94.58, average distance 306.71 -> 135.53, with four accepted roads.
SHA-256 checks confirm all six protected core implementation files are unchanged.


## Phase 7.3: Urban Composition

Detailed mode uses nonzero-alpha bounds measured once at texture load. Asset
preparation audits every PNG and retains only a one-pixel sampling gutter without
removing visible pixels. Sprites fit within a 2% margin on each side of the exact
building footprint. Closest-aspect selection is stricter. For footprints with
aspect ratio greater than 2:1, up to three adjoining sections of the same roof
can improve occupancy, preserving each section's aspect and the single unchanged
simulation building. Hospital artwork remains a single sprite. The 80-90% area
target is approximate: aspect mismatch can still leave more space; no stretching
or footprint enlargement is used to force that target.

`VisualComposition` draws full-footprint lawn, pavement, or plaza parcel bases.
Each existing block receives the palette of its most numerous footprint category
(deterministic ties favor the smaller category). Existing roof-category families
remain consistent, with deterministic variations inside each family. No simulation
zoning or parcel objects are created.

Narrow entrance approaches join the nearest feasible perpendicular road sidewalk.
Connections longer than 110 units, crossing buildings/road surfaces, or intruding
on facility markers are skipped. Continuous inner block edging meets the existing
road sidewalks. Planting uses groups of two or three trees on small landscaped
patches, capped at twelve clusters / thirty-six trees and at most two groups per
block. Clearance checks cover buildings, paths, facilities, block boundaries, and
other planting. The simplified toggle retains the earlier procedural view. UI,
roads, graph, facility associations, routes, scoring and optimization are unchanged.

Validation: all 12 existing tests pass without test modifications. Before/After
framebuffer captures confirm contained roofs and readable analytical overlays.
A local hidden-window city-only benchmark (seed 42, 200 frames, glFinish per frame,
no VSync/UI) measured approximately 1.38 ms detailed versus 0.27 ms simplified;
these figures are machine-specific, not an end-to-end FPS guarantee. All protected
algorithm hashes match. Seed 42 remains 87.73 -> 94.58 with four accepted roads.
No new artwork or dependencies were added. Phase 7.3 is the final authorized polish.


## Final Art Direction: Clean City-Planning Map

This renderer supersedes the sprite-heavy presentation described in the earlier
visual-phase notes. Detailed City Visuals now uses procedural architectural forms
as its base. Buildings occupy about 92% of their unchanged footprint area: warm
neutral residential bodies with terracotta/slate roofs; tan commercial buildings;
and cool blue-gray offices. Hospital bodies use pale teal/white. Existing textures
are reserved for readable hospitals and a small deterministic subset of large
commercial/office landmarks. No new artwork was added.

Projected-size detail gates suppress roof ridges below 22 pixels, commercial/office
insets below 26 pixels, and roof equipment below 42 pixels on the short side.
Ordinary buildings never use tiny PNGs or repeated sprite roof sections in this
view. Turning Detailed City Visuals off shows basic footprint rectangles and road
centerlines, retaining analytical overlays.

Asphalt is now 13 world units wide (previously 8), with 15-unit curbs/accent borders
and 18-unit sidewalks. These are visual strips only; the mathematical centerlines
are unchanged. Dashes stop near intersections. Entrance paths end at the new
sidewalk edges. Ground palettes still reflect the dominant existing footprint
category per block. Vegetation is reduced to six small groups at most (18 simple
polygon canopies), with no tree sprites or micro-props.

The orthographic camera fits the city boundary with a small margin and preserves
aspect ratio. Graph mode dims city artwork by 34% before drawing graph edges/nodes;
facility markers and the stronger five-unit gold route are drawn afterward.
Accepted roads keep their ordinary asphalt with a restrained teal border.

Validation: all 12 existing tests pass unchanged, including seed 42's
87.73 -> 94.58 and four accepted roads. The UI and all simulation code remain
unchanged. Framebuffer screenshots were inspected at 1280x800, 1600x900, and
1920x1080, including Before/After, selected route, and graph overlay.
## Phase 7.4: Current viewport rendering

This section supersedes the visual dimensions, detail thresholds, and vegetation
limits in the historical notes above. Simulation geometry and the application UI
are unchanged.

- `VisualRoadStyle.h` centralizes presentation-only road widths and road-facing
  orientation. Corridors target up to 30 world units, constrained by existing
  building clearance and parallel-road spacing. Sidewalks, curbs, and asphalt
  render in separate passes so intersections merge; center dashes stop at crossings.
  Dense areas necessarily retain narrower streets to preserve existing footprints.
- `Renderer.cpp` draws substantial vector building bodies, residential hip roofs,
  tan commercial forms, and slate office forms. Bodies occupy about 92% of each
  original footprint area. Entrances face the nearest road. Roof insets appear at
  16 projected pixels, entrances at 20, and rooftop equipment at 36.
- Existing hospital textures appear above 30 projected pixels; occasional large
  commercial/office landmarks use existing assets above 36/44 pixels respectively.
  Ordinary buildings use vector forms. No new artwork or dependencies were added.
- `VisualComposition` grows decorative lawn/pavement/plaza parcels around existing
  buildings, checking block bounds, roads, other footprints/parcels and facilities.
  Feasible perpendicular entrance paths connect parcels to sidewalk edges.
  These are cached paint commands, not simulation objects. The cache invalidates
  when geometry, facilities, or the parking-detail level changes.
- Deterministic planting is capped at ten groups, one per block, including at most
  three larger park patches. Simple two-tone polygon canopies replace tiny tree
  textures. Placement checks avoid roads, buildings, facilities and entrance paths.
  Parking marks disappear below 1.2 screen pixels per world unit.
- The selected route has a six-unit gold center over a dark understroke. White
  hospital frames/crosses and outlined coral incident diamonds remain prominent.
  Accepted roads retain restrained teal accents. Graph mode dims the underlying
  city by 28%; analytical overlays render above it.
- Detailed City Visuals can still be disabled for basic footprints and centerlines.

Validation: stages A-F were built and visually inspected. All 12 existing tests
passed unchanged, including the three graphics/demo smoke tests. The actual demo
also launched successfully. Protected simulation source hashes remain unchanged.
Seed 42 retains 87.73 before, 94.58 after, and four accepted connector roads.

A local hidden-window benchmark measured approximately 0.35 ms per detailed city
frame and 0.11 ms simplified (200 frames after warmup, glFinish each frame,
1280x800 window, no VSync or UI). This is a machine-specific city-render measurement,
not an end-to-end FPS guarantee. Road widths are calculated once per draw and
landscape composition is cached across unchanged frames.

Final captures: [1280x800 Before](docs/screenshots/phase74-before-1280.png),
[1600x900 Before](docs/screenshots/phase74-before-1600.png), and
[1920x1080 After with graph](docs/screenshots/phase74-after-graph-1920.png).

Run the current build from the project directory:

```powershell
.\build\ProceduralCityGenerator.exe --demo
```
## Building appearance: Stage 1 (current)

Detailed City Visuals now uses existing residential, commercial, office and
hospital PNGs as its first choice. Procedural buildings are fallbacks for missing
assets or extreme aspect mismatches. No source atlas or runtime PNG was recropped.
This replaces Phase 7.4's landmark-only texture selection.

`VisualComposition` caches one render-only `BuildingArtwork` record per simulation
building. The envelope grows in deterministic two-unit steps, up to sixteen units
per side, within the containing block. Growth checks road corridors, other
simulation footprints, previously reserved envelopes and unrelated facilities.
A building's own facility marker does not prevent its artwork from growing.

`VisualAssets::layoutBuilding` classifies using the original footprint and selects
an aspect-compatible existing asset/rotation with a stable geometry hash. It fits
one complete sprite into the available envelope without stretching or repeated
roof tiles. The painted property is compacted to that aspect, with artwork using
90% of each dimension (81% of property area). Extreme aspect mismatches below
35% envelope utilization fall back to procedural rendering. Facility coordinates
and simulation geometry are never written by these presentation calculations.

Seed 42 Before: 23 residential, 42 commercial, 23 office and 3 hospital sprites;
zero fallbacks. Mean artwork area is 2.58 times the original simulation footprint
area. This is an average of per-building ratios, not whole-city coverage.
All 12 existing tests pass unchanged. Additional local audits passed containment,
road/neighbor clearance, preserved aspect ratios and deterministic rebuilding for
seven Before/After/seed layouts. Seed 42 remains 87.73 -> 94.58 with four accepted
roads. Protected simulation source hashes match.

[Normal overview, 1280x800](docs/screenshots/stage1-buildings-1280.png) and
[normal overview, 1600x900](docs/screenshots/stage1-buildings-1600.png).

Roads, intersections, vegetation composition, parks, parking and UI were not
redesigned in this stage. Further visual stages await building-appearance review.
## Streets: Stage 2 (current, awaiting visual review)

The approved Stage 1 building artwork, envelopes, asset selection and scale are
unchanged. `StreetPresentation.h` creates presentation-only width profiles from
existing road centerlines and the original footprints/current artwork bounds.
It does not write simulation or building presentation data.

Corridors target a full width of 32 world units, limited by parallel streets and
building clearance (0.75 units). Conservative samples at intervals no larger than
six units protect the whole interpolated strip. Width is held consistent between
intersections; short inward-only tapers join different widths without introducing
wavy sidewalk edges. Empty property margins may be used; buildings remain clear.

The street pass paints all concrete sidewalks, then all curbs, then all charcoal
asphalt, merging intersection surfaces without internal border seams. Asphalt uses
73% of corridor width; curbs extend to 79%, leaving proportional sidewalks outside.
Center dashes are seven units long at eighteen-unit intervals, suppressed around
junctions and crosswalk approaches. Zebra crossings appear only on sufficiently
wide, long and separated approaches. Optimizer roads use the same geometry with a
thin teal curb accent. The selected graph route retains its exact path, with a
seven-unit gold stroke over a nine-unit dark understroke.

Seed 42 Before corridor widths span approximately 16.6-32 world units. Length-
weighted average asphalt width increased from 12.08 to 14.58 units (about 21%).
This is clearance-limited; not every street can be widened to the maximum while
preserving the approved buildings.

Validation: all 12 existing tests passed unchanged. A local audit checked complete
street-strip clearance against original building footprints and presentation
artwork, centerline preservation and deterministic widths across seven layouts.
The Stage 1 building audit still passes: seed 42 Before retains 23 residential,
42 commercial, 23 office and 3 hospital sprites, with 81% fitted-property coverage.
All 20 protected building/simulation/test source hashes match. Seed 42 scoring
remains 87.73 -> 94.58 with four accepted roads. Normal demo launch succeeded.

[Normal view, 1280x800](docs/screenshots/stage2-streets-1280.png),
[normal view, 1600x900](docs/screenshots/stage2-streets-1600.png), and
[After with graph, 1920x1080](docs/screenshots/stage2-streets-after-graph-1920.png).

Stage 2 changes: `src/Renderer.cpp`, new `include/StreetPresentation.h`, this
README, and the three captures above. Local verification tools/logs are under
`.tools/`. No Stage 3 landscaping, parking, vehicle or shadow work is included.
## Stage 2.1: Intersection correction

Removed the repeated zebra-crossing bars entirely. Ordinary intersections retain
merged asphalt, the existing clear center-dash approach gaps, sidewalks and curbs.
Road widths, building presentation and lane markings outside intersections are
unchanged. No replacement crossing objects or simulation changes were added.
Build and all 12 regression tests passed; seed 42 remains 87.73 -> 94.58 with four
accepted roads. [Updated normal view](docs/screenshots/stage21-intersections-1600.png).
## Properties: Stage 3 (awaiting visual review)

Property composition is rendering-only and cached. Approved building envelopes,
artwork selection/scale and Stage 2.1 street rendering remain unchanged.

- Houses receive modest lawn margins and narrow concrete entrance paths.
- Commercial properties use warm neutral pavement and small street-facing aprons.
- Office properties use cooler paved frontage and restrained planting strips.
- Hospitals retain their artwork and markers, with wider, lighter entrance paths
  where clearance permits.
- Optional narrow driveways and small two-space parking sections use deterministic
  geometry-based selection. Parking marks appear only at readable screen scales.
- A maximum of six clear gaps receive subtle shared grass surfaces with clipped
  corners and no outlines. Existing tree-group limits are unchanged; no new tree
  assets or dense planting were introduced.

Each entrance considers perpendicular connections from the center and quarter
points of each facade to existing road profiles. The nearest feasible sidewalk
connection is chosen. Checks protect neighboring property surfaces, simulation
footprints, artwork, road corridors, previous decorations and facility markers.
Paths remain within their existing block. Residential connections longer than
75 units and other connections longer than 110 units are omitted. Building and
facility coordinates, graph attachments and route distances are never changed.

The modest ground margin is capped at three world units and the original reserved
presentation envelope. This does not enlarge or rescale building artwork. Paint
commands are cached until geometry, facilities or the parking-detail level changes.
Analytical overlays continue to render above property decoration.

Validation: build and all 12 existing tests passed. A separate local audit compared
building bounds, fitted properties, asset choices and street profiles with the
pre-Stage-3 implementation across seven layouts: no differences. It also verified
deterministic composition rebuilding and entrance clearance from buildings, roads
and markers. All 20 protected building/renderer/simulation/test source hashes match.
Seed 42 Before has 77 sidewalk connections and one shared grass treatment; scoring
remains 87.73 -> 94.58 with four accepted roads. Normal demo launch succeeded.

[Normal detailed view, 1600x900](docs/screenshots/stage3-properties-1600.png),
[1280x800 view](docs/screenshots/stage3-properties-1280.png), and
[After with analytical overlays](docs/screenshots/stage3-properties-after-graph-1920.png).

Production code modified: `src/VisualComposition.cpp` only. Documentation and these
screenshots accompany it; local baseline/audit artifacts are under `.tools/`.
Stage 4 vegetation/environment work is not included.
