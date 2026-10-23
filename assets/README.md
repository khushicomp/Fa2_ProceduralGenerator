# Visual Asset Inventory

The runtime uses 19 individual PNGs, never the complete source atlas:

- buildings/residential/01.png through 06.png: six roof variants.
- buildings/commercial/01.png through 04.png: four flat commercial roofs.
- buildings/office/01.png through 04.png: four larger office/civic roofs.
- buildings/hospital/01.png: one hospital roof.
- vegetation/trees/01.png through 04.png: four canopy cutouts.
- source/city-atlas.png: original user-supplied Top-Down City Asset Atlas.png,
  retained only for reproducible extraction. Not loaded by the application.

Run tools/prepare-assets.ps1 from Windows PowerShell to reproduce all crops.
The script uses System.Drawing. Building crops tightly isolate rectangular roofs,
excluding adjacent atlas content and the opaque olive background. Tree crops use
ellipse masks inside the canopy. Each PNG is alpha-audited and cropped to one transparent sampling pixel around its visible bounds.
The runtime uses the visible bounds for aspect matching and texture coordinates.
No unfinished sprite preparation is required for the enabled assets.

The supplied sheet is the source; no third-party stock-art license is asserted.
Its original source/provenance remains the user's supplied artwork.

Preparation experiment: the built-in imagegen tool was asked to preserve the
1536x1024 atlas exactly while replacing only its continuous outside background
with transparent alpha. That result retained the background and was rejected.
The final runtime assets are deterministic tight crops/masks of the original
sheet, not that generated experiment. No generated alternative is referenced.

Preparation prompt: "Edit this supplied city asset atlas ONLY to remove the olive
background between separate sprites, replacing it with real transparent alpha.
Preserve precisely all sprite positions, count, sizes, roofs, architecture, colors,
and sheet layout on the same 1536x1024 canvas. No new objects, no rearrangement,
no labels. Keep self-contained building lots and garden details inside each
building tile; remove only continuous background outside tiles and around
standalone trees/props. This is a technical game-asset preparation sheet for
later cropping into individual PNGs."

Markers, roads, sidewalks, muted terrain, and rare park rectangles continue to
use procedural OpenGL primitives. Cars, benches, lamps, fountains, and ponds are
not enabled. There are no empty placeholder asset folders.
