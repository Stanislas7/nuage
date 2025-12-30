# Terrain Visual + Performance Diagnosis (Nuage)

Goal: Reach a "vanilla X-Plane / P3D" look while keeping strong, stable performance.
This document captures the desired outcome, current gaps, and an optimization-first
path that improves quality without relying on down-resing.

## Targets
- Visual feel: realistic, sim-like; not stylized; consistent at all altitudes.
- Performance: stable frame time under camera movement, low stutter, predictable VRAM.
- Scalability: quality increases should be bounded by explicit budgets.

## Success metrics (baseline to define)
- Frame time at 1080p and 1440p (CPU/GPU).
- VRAM use with N=2 and N=3 tile radii.
- Tile load time budget per frame (streaming).
- Visible tiling artifacts score (subjective, but captured via reference shots).

## Current system (from terrain_pipeline.md)
- Offline compiled grid tiles, 2-level LOD, skirts.
- Mask-driven landuse (water/urban/forest/grass) optional.
- Runtime shading: noise tint, altitude tint, slope darken, distance haze.
- Textures: grass/forest/rock/dirt/urban with macro/micro variation.
- Trees: simple procedural placement based on masks.

## Recent fixes (ground/terrain)
- JSBSim ground callback now samples compiled/procedural terrain (including runways) and feeds the real surface normal instead of a spherical normal; cached last sample prevents hard zero-height fallbacks.
- Terrain sampling is unified via `TerrainRenderer::TerrainSample` (height + normal + mask weights + runway flag) so physics/collision callers can share one codepath.
- Physics preload now pulls the surrounding tile ring for JSBSim to avoid missing height data during streaming.

## Visual gaps vs X-Plane / P3D
- LOD blending: hard swaps vs geomorph / dithered transitions.
- Material richness: no normals/roughness/decals; limited microstructure.
- Landclass variety: only 4-5 classes; no region-specific palettes.
- Vector detail: no roads/rails/coastlines/rivers.
- Water: flat color + noise, no shore blend or wave detail.
- Autogen: no buildings/roads/field patterns.
- Atmosphere: limited scattering/lighting response; no seasonal changes.

## Performance risks (if res is pushed without optimization)
- Vertex bandwidth and draw calls increase superlinearly with grid size.
- Streaming stalls if disk IO or CPU decode spikes.
- Mask sampling and texture fetch cost rise with more materials.

## Optimization-first upgrades (no down-resing)

### 1) LOD strategy upgrade
Objective: keep high-res near the camera and reduce far cost without pops.
- Add geomorph between LOD0/LOD1 (vertex morph or screen-space dither).
- Use 3-4 LOD levels if budget allows; LODs chosen by projected error.
- Option: continuous LOD (clipmaps) to keep triangle budget stable.

### 2) GPU-friendly mesh layout
Objective: improve cache locality and reduce bandwidth.
- Pre-indexed grid with consistent winding.
- Quantize heights and normals in mesh data (e.g., 16-bit) with shader decode.
- Keep vertex format tight (pos.xy + height + packed normal).

### 3) Material system depth (cheap realism)
Objective: add richness with minimal shader cost.
- Add normal/roughness maps for base textures.
- Use tri-planar only on steep slopes (avoid global cost).
- Add detail normals (cheap tileable) blended by slope/landclass.
- Use decals sparingly for roads/dirt tracks in dense areas.

### 4) Landclass expansion (procedural)
Objective: more believable variety without GIS.
- Derive biome masks from height/slope/moisture noise.
- Expand to 8-12 classes (rock, alpine, scrub, farmland, sand, wetland, etc.).
- Region presets (California coast vs Sierra) via config.

### 5) Water improvement
Objective: believable water at low cost.
- Shoreline blend based on height threshold + noise.
- Simple normal map + fresnel + sun glint.
- Distance-based flattening for far water (reduces shimmer).

### 6) Streaming and CPU budget
Objective: reduce stalls when moving fast.
- Preload rings (2N+1 plus one extra ring) with low priority.
- Cap loads per frame; keep async IO and CPU decode separate.
- Optional: mesh cache on disk (post-processed/quantized).

## Roadmap proposal (phased)

### Phase 1: Quality with minimal risk
- Add geomorph or dithered LOD transitions.
- Add normals/roughness to terrain textures.
- Expand landclass to 8-12 using procedural masks.
- Add shoreline blend and a simple water normal.

### Phase 2: Performance stability
- Quantize vertex data; shrink vertex format.
- Add LOD error metric for better swaps.
- Add streaming budget telemetry and on-screen stats.

### Phase 3: Sim-like richness
- Region presets (config packs).
- Procedural roads/fields (rule-based).
- Autogen-like vegetation clusters (biome-based).

## Open questions (to finalize)
- Target hardware (GPU class, CPU).
- Desired minimum FPS and resolution.
- Acceptable CPU/GPU split for terrain rendering.
- Visual reference screenshots (X-Plane/P3D areas you want to match).
