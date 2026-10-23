# Complete project documentation audit

Audit date: 2026-10-10. Scope: the current local workspace through property-composition Stage 3.

## Result

The core pipeline is documented accurately, and the documented existing-workspace build and test commands work. The documentation needs consolidation before it can serve as an unambiguous current project reference. The main problem is accumulated, superseded rendering descriptions, not an incorrect explanation of BFS, Dijkstra or the accessibility score.

No production source, existing documentation or test was changed for this audit. This report is the only new documentation file. Build artifacts and an audit build log were produced by verification.

## Scope and evidence

Reviewed [README.md](../README.md), [asset inventory](../assets/README.md), [CMakeLists.txt](../CMakeLists.txt), public data structures, application/UI controls, generation, graph construction, facilities, routing, scoring, optimization, rendering, asset loading, composition, tests and the asset-preparation script. Inspected the local capture/audit tooling under `.tools/` for reproducibility claims.

Evidence locations below use line numbers as observed on the audit date. Historical notes can remain useful, but should be explicitly separated from current operating documentation.

Verified in this audit:

- Existing MinGW Debug build succeeded using the README's workspace CMake command.
- All 12 CTest tests passed in 12.64 seconds, including the actual graphics smoke tests.
- `--help` matches the documented options.
- All 13 relative file links in README resolve; the asset README has no Markdown file links.
- Local tools report GCC 14.1.0 and CMake 3.31.6, matching the documented compiler/CMake versions.
- Fresh test output retains the documented candidate and seed-42 optimization results.

Not newly verified: a clean Internet-based dependency download, MSVC build, another computer/driver, physical mouse/keyboard input, historical performance measurements, or every historical manual visual inspection. The existing build uses cached GLFW and ImGui source overrides under `.tools/`; its success does not prove a clean online configure.

## Findings and corrections

### D01 — HIGH: Multiple incompatible descriptions present themselves as current

Evidence: README lines 369, 435, 471, 503, 556 and 590 introduce successive visual systems. Lines 474-479 and 514-520 say vector architectural forms are primary and textures are rare landmarks. Lines 439-445 describe footprint-contained repeated roof sections. Lines 606-608 describe zebra crossings. Stage 1 replaces the building rules, and Stage 2.1 removes the crossings.

Actual behavior: `Renderer.cpp:151` attempts asset drawing first; `VisualAssets.cpp:72` selects one building sprite for a bounded presentation envelope. `Renderer.cpp:49` merges road surfaces and leaves junction gaps without crosswalk bars. `StreetPresentation.h` controls displayed street width.

Impact: readers can correctly follow one section and still describe the current application incorrectly.

Correction: make one current rendering section cover approved buildings, Stage 2.1 streets and implemented Stage 3 composition. Move older descriptions, screenshots and benchmarks into a clearly historical appendix. Remove competing “current” and “final” headings from the main guide.

### D02 — MEDIUM: Asset inventory confuses loaded assets with displayed assets

Evidence: `assets/README.md:3` says the runtime uses 19 PNGs. README lines 391-394 describe a 0.04 log-aspect tolerance and containment in the original footprint.

Actual behavior: `VisualAssets::initialize()` loads 6 residential, 4 commercial, 4 office, 1 hospital and 4 tree PNGs. Building groups are eligible for drawing, but `visual::tree()` has no application rendering caller; current canopies are polygons. `layoutBuilding()` uses a 0.10 near-best log-aspect tolerance and a render-only envelope. Assets remain one sprite each; the source atlas is not a runtime texture.

Correction: distinguish available, loaded and actually rendered assets. Document the current selection tolerance and envelope behavior once. Retain the accurate description of roof-only crops and WIC loading. Do not imply all atlas roads, vehicles or landscape props are implemented.

### D03 — MEDIUM: The detailed-mode toggle is described incorrectly

Evidence: README lines 410-414 say disabling Detailed City Visuals restores procedural roofs/canopies and discuss missing tree sprites being skipped.

Actual behavior: `Renderer.cpp:145` skips property composition when detailed mode is off; `Renderer.cpp:156` draws plain building footprint rectangles; `drawStreets()` uses simple thin road strips. Facility and analytical overlays remain. Detailed mode itself uses polygon canopies, not tree textures.

Correction: say “basic footprint rectangles and road centerline strips, without detailed property decoration; analytical overlays remain.” Separate this intentional simplified mode from the procedural fallback used when an asset is missing/unusable in detailed mode.

### D04 — MEDIUM: Decoration limits and marker descriptions are stale

Evidence: README lines 209 and 414 state 24 trees/four patches; lines 457-458 describe twelve groups/thirty-six trees; lines 491-492 describe six groups/eighteen canopies. The main appearance table describes gold backing squares for selected facilities without distinguishing incident diamonds.

Actual behavior: `VisualComposition.cpp:264` caps legacy planting at ten groups, one per block. At most three are park groups with four canopies; other groups contain two or three. The theoretical maximum is therefore 33 canopies, not a guaranteed actual count. Stage 3 additionally allows up to six shared grass areas; these are separate from legacy park groups and contain no newly added trees. `Renderer::drawEmergency()` uses a gold square for the selected hospital and a gold diamond for the selected incident.

Correction: publish one current table of limits, distinguish group limits from actual placements, and distinguish grass treatments from tree/park groups. Correct the marker legend.

### D05 — MEDIUM: The main testing section lacks a current test inventory

Evidence: README line 346 reports eleven tests for historical Phase 7, while later sections report twelve. The main section does not include VisualAssetTests in its consolidated inventory.

Actual behavior: CMake registers eleven general tests plus VisualAssetTests on Windows when BUILD_TESTING is enabled. There are twelve in this workspace.

Correction: list the current twelve tests and their coverage together. Label old eleven-test results historical. State that VisualAssetTests is conditional on Windows and that graphics smoke tests require a usable OpenGL desktop/context even with hidden windows. Retain the existing distinction between synthetic UI events and physical interaction testing.

### D06 — MEDIUM: Several validation claims depend on ignored, undocumented tooling

Evidence: `.gitignore` excludes `.tools/` and build directories. README cites presentation hashes, seven-layout audits, coverage ratios and framebuffer inspections. The supporting `PropertyCompositionAudit.cpp`, baseline copies, hash manifests, `GuiCapture.cpp` and `preview-stage.ps1` are local ignored artifacts.

Actual behavior: the standard CTest suite is reproducible from source, but it does not contain the complete later presentation-baseline audits or screenshot capture workflow. The screenshot helper compiles a separate instrumented copy of main and uses Python to encode captures.

Correction: distinguish standard regression coverage from supplemental local audits. If those visual claims are to be independently reproducible, retain a minimal helper and documented invocation in `tools/`, together with the relevant baseline definition. Otherwise mark them as historical local observations. Python is not required to build the application; it is needed for the current optional local screenshot helper.

### D07 — MEDIUM: The architecture explanation omits the current presentation pipeline

Evidence: README lines 219-224 summarize orchestration/UI responsibilities but omit the relationship between the two road-width helpers, cached artwork layout, composition and texture lifetime.

Actual behavior:

```text
Generation parameters
  -> CityGenerator -> CityData
  -> RoadGraphBuilder -> RoadGraph
  -> placeFacilities -> Facilities
  -> AccessibilityAnalyzer -> route results
  -> AccessibilityScorer -> score
  -> CandidateLayout stores original and optional optimized snapshots

Selected snapshot (read-only)
  -> VisualComposition: approved envelopes + cached property paint
  -> VisualAssets: asset selection/layout and textured quads
  -> StreetPresentation: clearance-limited street profiles
  -> Renderer: city, graph overlay, route/facility markers
  -> ApplicationInterface: controls and stored metrics
```

`VisualRoadStyle.h` still supplies conservative envelope clearance and fallback-facing logic. `StreetPresentation.h` supplies actual displayed street profiles. They are not interchangeable definitions of simulation road geometry.

Correction: add this module/data-flow map and the lifetime rule: texture creation/drawing/destruction requires the active OpenGL context, while simulation modules do not depend on rendering. Describe render-only property records separately from simulation Building/Block/Road data.

### D08 — MEDIUM: Computer Graphics concepts are missing from the project explanation

Evidence: there is no dedicated explanation of transformations, projection, texture coordinates, clipping and layering, despite this being a Computer Graphics project.

Verified concepts to document:

| Concept | Actual implementation |
| --- | --- |
| Translation | Local sprite/marker offsets are added to world-space centers; no explicit object `glTranslatef()` path |
| Scaling | Aspect-preserving contain scaling and viewport/world scaling; no claim of nonuniform sprite stretching |
| Rotation | Quarter-turn sprite orientation through UV order and swapped dimensions; not general arbitrary-angle object rotation |
| Projection | Fixed-function `glOrtho()` for the top-down Cartesian view |
| Viewport | Aspect-preserving letterboxing plus framebuffer/DPI scaling |
| Clipping | OpenGL viewport/scissor confines the map to the city panel; geometry clearance is a separate CPU check |
| Primitives | Quads for sprites/roads/surfaces, triangles for fallback roofs, triangle fans for polygon canopies, lines/points for graph overlays |
| Transparency | RGBA PNGs and source-alpha blending |
| Layering | Ground/property paint, buildings, streets, optional graph overlay, then route/facility markers; ImGui is drawn afterward |
| Rasterization | OpenGL rasterizes submitted primitives; custom DDA/Bresenham algorithms are not implemented |

Correction: add a concise CG concepts section with source examples. Do not claim explicit transformation matrices, custom line rasterizers, perspective/3D rendering or shader features that the application does not implement.

### D09 — MEDIUM: Coverage metrics need explicit denominators

Evidence: README contains 80-90%, 92%, 81% and 2.58x figures across different generations of the renderer.

Actual current meaning: 81% is the rectangular artwork area divided by its fitted BuildingArtwork.property area (0.9 x 0.9). Stage 3 can paint a slightly larger surface within the reserved envelope, so 81% is not necessarily coverage of every visible lawn/pavement surface. The 2.58x figure is a seed-42 Before average of individual artwork/original-footprint area ratios, not overall viewport density. Hospital skins override categories when a facility is contained in a footprint; category counts are snapshot-specific.

Correction: retain each current metric only with its denominator, snapshot, seed and settings. Mark old footprint-coverage targets as historical. Do not describe presentation enlargement as mutation of simulation geometry.

### D10 — LOW: Build/run guidance works, but troubleshooting and validation boundaries are incomplete

Evidence: README lines 245-291 provide sensible separate MSVC and MinGW directories, PATH setup, runtime DLL guidance and offline source overrides. The workspace command assumes a pre-existing configured build and local tools. There are no checked-in VS Code tasks/presets; `.vscode/` is ignored.

Correction: clearly distinguish clean setup from rebuilding this workspace. Add short troubleshooting entries for compiler/generator mismatch, GitHub fetch failures, missing runtime DLLs, OpenGL-context failure and missing executable-adjacent assets. Explain that the command-line workflow works in VS Code's terminal without supplied tasks. Keep MSVC instructions labeled as a supported configuration not newly validated by this audit.

### D11 — LOW: Completion/approval labels and old source comments are misleading

Evidence: README lines 423 and 468 call older phases final; line 590 still says Stage 2 awaits review. Session approvals establish Stage 1 and Stage 2/2.1 as approved; Stage 3 has been implemented but not explicitly visually approved. `CityGenerator.cpp:64` says graph splitting is deferred even though a separate RoadGraphBuilder stage now performs it.

Correction: add one status line: “Core pipeline complete; buildings and streets approved; property composition implemented and awaiting visual review; Stage 4 not started.” Archive phase-completion statements. Clarify that graph construction is delegated to RoadGraphBuilder rather than an unimplemented future feature.

### D12 — LOW: Algorithm explanations omit implementation-level complexity and contracts

The algorithm summaries are accurate, but a college report would benefit from input/output contracts, concise pseudocode and complexity notes.

- BFS is O(V + E) per incident, plus collecting reachable hospitals.
- Heap Dijkstra on this simple graph is O((V + E) log V) per incident, plus hospital selection and path reconstruction. The analyzer also sorts reachable hospital IDs.
- RoadGraphBuilder tests road pairs and then scans all nodes for each road; do not describe it as merely linear in the final graph size. Ordered map/set operations add logarithmic factors.
- Optimizer cost includes repeated graph rebuilding, nearest-node facility reassociation, BFS, Dijkstra and scoring for each attempted proposal.
- `rebuildFromRoads()` serves the full-span axis-aligned city subdivision; arbitrary imported road networks are outside the documented model.

Correction: add a short technical appendix, keeping estimates separate from measured timings and stating R (roads), V (nodes), E (edges), incident count and attempt count.

## Documentation that should be retained

The following explanations agree with the inspected implementation:

| Topic | Verified behavior |
| --- | --- |
| Procedural generation | Seeded slot selection/jitter, 40-unit default spacing, density ranges, inset rectangular blocks and independent building random stream |
| Graph | Endpoint/crossing nodes, stable coordinate-based IDs, deduplicated undirected edges and bidirectional adjacency with Euclidean weights |
| Facilities | Building centers preferred, then block/graph sites; nearest-node attachment; insufficient sites reduce counts |
| BFS/Dijkstra | Explicit BFS, weighted shortest paths, lowest hospital ID on equal distances, predecessor reconstruction, empty path for unreachable incidents |
| Score | 60% reachability + 40% normalized average reachable road distance; 1000-unit boundary diagonal; access connectors excluded |
| Candidates | 1-5 consecutive unsigned seeds, pending edits, original-score comparison, independent stored optimization results |
| Optimization | Add-only midpoint proposals, strict epsilon improvement, fixed facility positions, possible reassociation, finite search and no global-optimum guarantee |
| Controls | R/O/B/P/G/arrows/Escape match main.cpp; ImGui keyboard capture suppresses shortcuts |
| Dependencies | C++17, GLFW 3.4, Dear ImGui 1.91.9b, bundled GLAD 0.1.36, OpenGL 2.1 and Windows WIC |
| Limitations | Educational model; no traffic, 3D or GIS; facilities may be outside rebuilt buildings; candidates may have different facility locations |

## Current regression results

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
| InvalidSeedTest | Pass (expected failure behavior) |
| VisualAssetTests | Pass |

Demo settings: base seed 42, road density 0.5, building density 0.5, three candidates.

| Metric | Value |
| --- | --- |
| Original candidate scores A/B/C | 87.73 / 91.90 / 85.46 |
| Highest original candidate | B, seed 43 |
| Seed-42 optimized score | 94.58 |
| Seed-42 average road distance | 306.71 -> 135.53 |
| Seed-42 accepted roads / attempts | 4 / 17 |

Commands actually exercised:

```powershell
& .\.tools\python\cmake\data\bin\cmake.exe --build build --parallel 4
& .\.tools\python\cmake\data\bin\ctest.exe --test-dir build --output-on-failure
.\build\ProceduralCityGenerator.exe --help
```

Current run evidence is in `build/Testing/Temporary/LastTest.log`; build output is in `.tools/documentation-audit-build.log`. These are generated local artifacts, not a portable documentation package.

## Recommended correction order

1. Consolidate the current README; archive historical visual descriptions and benchmarks.
2. Correct asset usage, simplified-mode behavior, decoration limits, crosswalk status and the twelve-test inventory.
3. Add the architecture/CG explanation and clarify metric denominators.
4. Document or retain reproducible supplemental audit/capture tooling and add build troubleshooting.
5. Refresh approval/status labels and selected source comments.

These corrections do not require new simulation or rendering features. They were identified, not applied, by this audit.
