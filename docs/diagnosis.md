# Scenery Pipeline Diagnosis: Quadtree Tiles & Automation

**Date:** March 5, 2025  
**Scope:** Scenery imagery/DEM ingestion, tiling, and runtime readiness

## 1. Current State
- Terrain uses a single heightmap + single albedo texture (`assets/config/terrain.json`).
- Large satellite textures force manual downsampling (SFO base albedo ~400MB).
- Scripts fetch imagery/DEM but do not generate LOD tiles or consistent manifests.
- Runtime has no streaming/LOD support for terrain textures or height tiles.

## 2. Goals
- Preserve native resolution without manual desampling.
- Split imagery + height into a quadtree tile pyramid for streaming.
- Keep a low-res preview output so the current renderer still works.

## 3. Constraints
- **Renderer limitations:** Current terrain renderer consumes a single heightmap + texture.
- **GPU limits:** Large textures blow up VRAM or exceed texture limits.
- **Geo alignment:** Imagery + DEM must stay in the same UTM projection and grid.
- **Tooling:** Pipeline depends on GDAL + external STAC sources (network access).
- **Asset size:** Multi-LOD tiles expand disk footprint; we need a consistent layout.

## 4. Direction
**Quadtree tiles** are the practical near-term win:
- Straightforward to generate with GDAL.
- Easy to stream by screen-space error or distance later.
- Works with both imagery and heightmaps on the same grid.

**Alternative (future):** Geo-clipmaps or virtual textures.
- Better GPU sampling continuity, fewer files.
- More engine work (clipmap ring updates, sparse textures).
Given the current renderer, quadtree tiles are the most achievable step.

## 5. Plan
### Phase 0: Pipeline automation (now)
- Add a single build script to run imagery + DEM downloads.
- Generate preview outputs for the current renderer.
- Build quadtree tile pyramids and a manifest per scenery.

### Phase 1: Asset metadata and configuration
- Formalize `manifest.json` fields (tile size, LODs, geo transform).
- Standardize folder layout under `assets/scenery/<name>/`.
- Generate `terrain_preview.json` so the current renderer can use previews.

### Phase 2: Runtime tile streaming
- Load `manifest.json` and implement a quadtree tile selector.
- Stream albedo + height tiles asynchronously (with cache + budget).
- Add seams/edge handling and LOD blending.

### Phase 3: Enhancements
- Tile compression strategy (JPEG/PNG, or GPU-ready formats).
- Optional clipmap/virtual texture upgrade.
- Tooling for incremental re-tile updates.

## 6. TODOs
- Decide tile size defaults (512 vs 1024) and max LOD depth.
- Confirm the `manifest.json` schema for terrain streaming.
- Add runtime support for tiled height + albedo sampling.
- Confirm coordinate conventions (Y flip, origin, and UV mapping).
- Add validation checks for tile seams and nodata handling.

## 7. Risks / Watchouts
- Misaligned DEM vs imagery projections cause texture drift.
- Large tile counts can explode disk usage without compression.
- Seams at tile boundaries if resampling differs per LOD.
- Current renderer needs a safe fallback (preview assets) until streaming lands.
