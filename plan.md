# Terrain Pipeline Redesign Plan (TerraGear-like)

Goal: offline compilation into runtime-friendly tiles, DEM-first, OSM-next.
Runtime is a pure consumer with strict performance budgets. No runtime GIS.

Defaults (v1)
- tileSizeMeters: 2000
- gridResolution: 129 (cells) => 130x130 verts
- visibleRadiusTiles: 1 (3x3)
- maxLoadsPerSecond: 2
- budgets: <= 9 visible tiles, <= 300k tris total, <= 12 draw calls

Non-goals (v1)
- No runtime GeoTIFF/VRT/OSM parsing or reprojection
- No quadtree/LOD pyramids
- No full terrain material system (debug shading allowed)
- No preview activation coupling

## Stage 0 (2-hour milestone): Runtime-only synthetic tiles
Purpose: guarantee visible progress without file formats or disk I/O.

Inputs
- Synthetic height function with deterministic seed
- Bounds in local ENU (meters)

Outputs
- In-memory tiles generated on demand
- Debug view: tile borders + tile IDs

Runtime changes (only)
- Implement tile selection for a fixed 3x3 working set around camera
- Generate tile meshes procedurally in memory on first touch
- Async mesh creation + GPU upload; no blocking I/O
- Enforce invariant: tile meshes are created once per tile lifetime
- Logging: tiles loaded/unloaded, tiles in memory, tris, draw calls

Compiler changes
- None required

Acceptance tests (done when...)
- 60 FPS stable while moving across multiple tiles
- Tile rebuilds per minute = 0 in steady flight (no re-creation without eviction)
- No blocking disk I/O on main thread
- Debug overlay shows correct tile IDs and borders

Top risks + de-risk
- Risk: per-frame rebuilds => add assert and counter for rebuilds
- Risk: stalls on mesh generation => use async tasks and frame budget limits

## Stage 1: Offline compiler writes tilepack (DEM ingestion)
Purpose: replace synthetic generator with real data on disk, keep runtime same.

Inputs
- GeoTIFF DEMs within bounds (USGS 3DEP or similar)
- Bounds in LLA
- tileSizeMeters, gridResolution

Outputs
- Tile pack on disk (manifest.json + tiles/*.mesh + tiles/*.meta.json)

Compiler changes
- Load DEM, reproject to local tangent plane, resample to grid
- Generate mesh tiles + per-tile metadata
- Write manifest.json with origin + bounds

Runtime changes
- Add tilepack loader (manifest + tile files)
- Keep tile selection, streaming, and budgets unchanged

Acceptance tests
- Visual terrain matches DEM region
- Same FPS and budgets as Stage 0
- Logging shows disk loads capped at maxLoadsPerSecond

Top risks + de-risk
- Risk: seam cracks => enforce shared edge sampling
- Risk: reprojection pitfalls => lock a single local CRS per pack

## Stage 2: OSM integration (water + landuse masks)
Purpose: add minimal landclass mask tiles for debug shading.

Inputs
- OSM PBF extract matching DEM bounds

Outputs
- Optional per-tile mask (8-bit landclass codes)

Compiler changes
- Rasterize water and landuse into tile-aligned masks
- Update manifest availableLayers to include "mask"

Runtime changes
- Optional debug shader: color by mask code

Acceptance tests
- Water bodies visible in debug shading
- No FPS regression or additional per-frame work

Top risks + de-risk
- Risk: OSM parsing complexity => use a single library and keep mask resolution low

## Stage 3: Optional imagery (offline only)
Purpose: optional texture layer, still offline compiled and streamed.

Inputs
- Imagery datasets (optional)

Outputs
- Texture tiles aligned to the same grid

Compiler changes
- Reproject + tile imagery offline

Runtime changes
- Optional texture loading if imagery exists

Acceptance tests
- Terrain still renders without imagery present
- Performance budgets unchanged

Top risks + de-risk
- Risk: VRAM bloat => enforce per-tile texture resolution cap
