# Terrain Improvement Plan (Target: X-Plane 9/10 era quality, no orthos)

This plan focuses on a compiled, data-driven terrain pipeline that approaches the look and feel of X-Plane 9/10 without orthophotos or custom scenery. The emphasis is believable landclass texturing, solid mesh fidelity, and consistent atmosphere, not photorealism.

## Success Criteria (Practical Targets)
- Terrain reads as real geography from pattern and silhouette.
- Landclass boundaries are soft (no hard blocks at tile edges).
- Water bodies and shorelines feel natural.
- Textures do not obviously tile at common altitudes.
- Trees and urban areas appear where expected, without heavy pop.
- Performance remains stable with a fixed tile window.

## Constraints
- No orthophotos or custom scenery packs.
- Compiled/offline pipeline only; no runtime GIS parsing.
- Keep the system debuggable and solo-dev friendly.

---

## Phase 0: Define Targets and Benchmarks (1-2 days)
**Goal:** Establish concrete visual and performance goals so changes are measurable.

- [ ] Capture baseline screenshots at multiple altitudes and times of day.
- [ ] Decide target altitude bands (e.g., 300m, 1500m, 6000m) to compare.
- [ ] Set performance budget (max tiles, load time per frame, FPS target).
- [ ] Create a simple before/after capture checklist.

---

## Phase 1: Data Strategy and Tiling (2-4 days)
**Goal:** Ensure the input data supports believable landclass and water.

**Data Needed**
- DEM: SRTM/USGS/NASADEM for mesh.
- Landcover: ESA WorldCover / Copernicus / NLCD for biomes.
- OSM: water, landuse, roads, buildings.
- Airport/runway data (already present).

- [ ] Choose a standard DEM resolution for LOD0 (e.g., 10-30m) and LOD1/2.
- [ ] Pick a landcover source and map classes to terrain materials (forest, crop, scrub, rock, wetland).
- [ ] Decide mask resolution defaults (e.g., 256 or 512) to reduce blockiness.
- [ ] Decide tile size (e.g., 2km) and ensure consistent alignment across DEM and masks.
- [ ] Document data sources and preprocessing steps.

---

## Phase 2: Offline Compiler Upgrades (1-2 weeks)
**Goal:** Produce higher quality tiles and masks that reduce visible artifacts.

### 2.1 Mesh + LOD
- [ ] Add multi-LOD output in the compiler (LOD0/LOD1/LOD2).
- [ ] Implement a stable LOD switching strategy (neighbor-aware, no cracks).
- [ ] Add optional mesh smoothing or anti-terracing where DEM is noisy.

### 2.2 Masks + Landclass
- [x] Increase mask resolution output (256/512) and add optional blur/feathering.
- [ ] Separate water/urban/forest/crop/rock into distinct channels if possible.
- [ ] Add a landclass priority order to avoid hard edges (water > rock > urban > forest > crop > grass).

### 2.3 Biome Classification
- [ ] Convert landcover classes into a small, curated biome set.
- [ ] Bake biome IDs per tile (for texture selection and tinting).
- [ ] Add elevation and slope modifiers for snow/rock thresholds.

### 2.4 Packaging
- [ ] Compress masks (single-channel or small atlas) to reduce IO.
- [ ] Add version metadata and source info into the manifest.

---

## Phase 3: Terrain Shader and Texture System (1-3 weeks)
**Goal:** Make ground textures blend naturally and avoid repeats.

### 3.1 Texture Set Discipline
- [ ] Curate a texture set for each biome (albedo + normal + roughness).
- [ ] Normalize color and roughness ranges across all textures.
- [ ] Add a fallback texture for missing classes.

### 3.2 Blending and Variation
- [x] Expose mask feather/jitter/noise controls to config.
- [x] Improve landclass edge feathering in the shader.
- [x] Add macro variation (large scale) and micro variation (fine detail).
- [ ] Add a slope-driven rock blend with soft transitions.
- [ ] Add wetness/darkening near water and in lowlands.

### 3.3 Distance Behavior
- [ ] Add distance-based desaturation and contrast falloff.
- [ ] Use lower-frequency detail at far distances to reduce shimmer.

---

## Phase 4: Water and Shorelines (1-2 weeks)
**Goal:** Make water look like water without orthos.

- [ ] Implement a dedicated water material or pass.
- [ ] Add animated normal map or procedural wave normals.
- [ ] Color water by depth/shore proximity (shallow to deep gradient).
- [ ] Add shoreline foam or damp sand band (subtle).

---

## Phase 5: Autogen (Trees and Urban) (2-4 weeks)
**Goal:** Add life and scale without custom scenery.

### 5.1 Trees
- [ ] Use landclass masks to place tree clusters in forest/woodland.
- [ ] Add LODs for tree rendering (billboard or low-poly).
- [ ] Add distance culling + density controls.

### 5.2 Buildings
- [ ] Use OSM building footprints when available.
- [ ] Add simple procedural building blocks for missing data.
- [ ] LOD or impostor for far distances.

### 5.3 Roads
- [ ] Render roads as textured ribbons or decals.
- [ ] Add road-only landclass suppression (no trees on roads).

---

## Phase 6: Atmosphere and Lighting (1-2 weeks)
**Goal:** Integrate terrain into the sky and reduce harshness.

- [ ] Improve distance haze with sun-aware scattering.
- [ ] Adjust ambient/diffuse balance for terrain readability.
- [ ] Add sky color sampling for fog blending.

---

## Phase 7: Tooling and Iteration (ongoing)
**Goal:** Make iteration fast and predictable.

- [x] Add debug overlays for landclass/mask visualization.
- [ ] Add per-tile stats (mask resolution, LOD level, blend weights).
- [ ] Add a quick screenshot script with fixed camera presets.
- [ ] Track performance over time (tile load time, FPS).

---

## Recommended Order (Best ROI)
1) Mask resolution + blending improvements.
2) Texture set normalization + macro/micro variation.
3) Water shader and shoreline treatment.
4) Multi-LOD mesh and stable switching.
5) Trees and basic urban autogen.
6) Roads and building detail.
7) Atmosphere tuning and polish.

---

## Notes on Expectations
- X-Plane 9/10 visuals are achievable without orthos if landclass masks, texture quality, and shader blending are strong.
- The biggest visible problems today are landclass blockiness and texture tiling, not mesh quality.
- This plan keeps the pipeline deterministic and easy to debug.
