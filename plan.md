# Rendering + Scenery Pipeline Critique and Plan

This is a blunt pass over the current tiled terrain pipeline. The goal is to
call out why it feels slow/inconsistent today and outline a pragmatic plan to
get it to "Infinite Flight" class streaming behavior.

## Snapshot (what we have now)
- The tile renderer loads meshes, textures, and height tiles synchronously on
  the render thread (`src/graphics/renderers/terrain_renderer.cpp`).
- Tile selection is a distance-ring scan over every tile in view per LOD, not a
  true quadtree or screen-space error selector.
- The build pipeline emits a quadtree-like tile pyramid on disk, but the runtime
  uses it as a flat grid with no parent/child fallback.
- `scripts/build_scenery.py` shells out to `gdal_translate` per tile, per level,
  serially.

## Critical issues (root causes)
1) **Synchronous I/O and mesh generation in the render loop**
   - `ensureTileLoaded()` does file I/O and mesh building in-frame. This causes
     frame stalls as soon as the camera moves into new tiles.
2) **No LOD fallback or coverage guarantees**
   - Tiles are only drawn if the exact target LOD has loaded. Missing tiles
     become holes; no "draw parent LOD until child ready."
3) **Height sampling always pulls from L0**
   - Height meshes sample `m_heightLayer.levels.front()` regardless of tile LOD.
     For 1m data this defeats the whole pyramid and keeps I/O heavy.
4) **Unbounded caches**
   - `m_tileCache` and `m_heightTileCache` never evict. Long flights will grow
     memory without bound.
5) **Missing tile thrash**
   - Failed height tile loads are not cached as "missing," so the loader retries
     repeatedly (per vertex in the worst case).
6) **Candidate scan cost scales with tile count**
   - Each frame iterates a rectangular grid of tiles per LOD. Large 1m datasets
     create thousands of candidates per frame.
7) **Tile pipeline is slow by construction**
   - `scripts/build_scenery.py` spawns a `gdal_translate` process per tile. For
     1m imagery this means tens of thousands of processes and heavy disk I/O.
8) **Config is currently forcing worst-case behavior**
   - `assets/config/terrain.json` sets `tileMaxLod = 0`, which locks to L0 only.

## Why Key West 1m "does not load"
- **L0 only**: `tileMaxLod = 0` means no coarse LODs, so every visible tile is
  the heaviest possible tile.
- **Load budget vs. view size**: `tileLoadsPerFrame = 16` and `tileMaxTiles = 1024`
  are too small for a 20km view at 1m/512px tiles. You will see holes for a long
  time, and they keep moving as the camera moves.
- **No fallback**: if L0 is not loaded, nothing else draws in its place.

## Plan (phased)

### Phase 0: Instrumentation + sanity checks (short, unblockers)
- Add a "tile audit" step that compares `manifest.json` vs on-disk tiles and
  reports missing tiles per level.
- Add runtime stats/logging toggles: number of candidates, tiles loaded this
  frame, cache sizes, load times.
- Fix the `--reuse-imagery` print bug in `scripts/build_scenery.py` (uses
  `albedo_utm` before it is defined).

### Phase 1: Make streaming sane
- Move mesh/texture loading off the render thread (job queue + async decode).
- Add LRU caches with size/bytes budgets for:
  - Height tiles (CPU memory)
  - Mesh tiles (GPU memory)
  - Albedo textures (GPU memory)
- Add a negative cache entry for missing tiles so we do not retry every frame.
- Sample height from the **matching height LOD** for the current tile LOD.
- Add LOD fallback: if a child tile is missing or still loading, draw its parent
  LOD tile so coverage is never broken.

### Phase 2: Fix LOD selection cost + stability
- Replace the distance-ring scan with a quadtree selector and frustum culling.
- Use a screen-space error metric for LOD selection instead of fixed distances.
- Add optional crack fixing or skirts between LODs to avoid seams.

### Phase 3: Build pipeline efficiency
- Replace per-tile `gdal_translate` calls with a batched tiler (e.g. `gdal_retile`
  or `gdal2tiles`) and enable GDAL multithreading.
- Consider building COGs + internal overviews and reading tiles via GDAL at
  runtime (fewer files, faster streaming).
- Add a manifest validation step after tiling (tile counts, bounds, format).

### Phase 4: Long-term improvements
- Evaluate clipmaps / virtual texturing to reduce tile counts and I/O pressure.
- Unify height + albedo streaming so LODs stay aligned.

## Quick config experiments (to make 1m usable today)
- Set `tileMaxLod` to >0 so coarse LODs can draw while L0 loads.
- Reduce `tileViewDistance` and/or increase `tileLoadsPerFrame` temporarily to
  get a stable coverage ring.
- Lower `tileMaxResolution` for mesh generation if stalls are too severe.
