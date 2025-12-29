# Terrain Pipeline (Offline Compiler + Runtime Streaming)

This document describes the current terrain system: what it does, how to run it,
and the tradeoffs. The pipeline is intentionally simple and offline-first.

## Why this exists
- Eliminate runtime GIS work (no GeoTIFF/OSM parsing in-engine).
- Make terrain deterministic and fast with a small working set.
- Keep the system solo-dev friendly and easy to debug.

## Big picture
**Offline:** `terrainc` compiles inputs into a tile pack on disk.  
**Runtime:** the engine streams a small tile window and renders indexed meshes with 2-level LOD + skirts.

```
DEM + OSM (offline) -> terrainc -> tiles/ + manifest.json -> runtime streamer
```

## Features (current)
- DEM-driven terrain mesh tiles (indexed grid).
- 2-level LOD for compiled tiles (hard swap).
- Skirts on tile borders to hide LOD seams.
- OSM water + landuse masks (used for texture blending and tree placement).
- Terrain shading pass: low-frequency color variation, altitude tint, slope darkening, and distance haze.
- Small, fixed working set (radius tiles).
- No runtime GIS or reprojection.
- Deterministic results with explicit bounds and parameters.
- Optional procedural trees (simple trunk + cone canopy).

## Non-features (v1)
- No runtime GeoTIFF/VRT/OSM parsing.
- No quadtree/LOD or pyramids.
- No imagery required or used.
- No full material system (debug colors only).

## Tile product (contract)
Output folder:
```
assets/terrain/compiled/
  manifest.json
  tiles/
    tile_X_Y.mesh
    tile_X_Y.meta.json
    tile_X_Y.mask   (optional)
```

### `tile_X_Y.mesh`
Binary format:
- Magic: `NTM1`
- uint32 floatCount
- float[] (pos.xyz, normal.xyz, color.rgb) per vertex (tri list)

Runtime note: tiles are converted to indexed grids on load for lower vertex bandwidth.

### `tile_X_Y.mask` (optional)
- Raw 8-bit raster of size `maskResolution x maskResolution`
- Values:
  - 0 = unknown/none
  - 1 = water
  - 2 = urban
  - 3 = forest
  - 4 = grass/crop

### `manifest.json`
Key fields:
- `tileSizeMeters`
- `gridResolution`
- `maskResolution` (if masks exist)
- `boundsENU`
- `availableLayers` (["height"] or ["height","mask"])
- `tileIndex` (list of [tx,ty] tiles present)

## Runtime streaming
- Uses a fixed radius (default 1 => 3x3 tiles).
- Loads at most a small number of tiles per frame.
- Tiles are created once per lifetime; evicted tiles free GPU memory.
- Compiled tiles use a 2-level LOD swap and border skirts (configurable).

Visible radius is configured in `assets/config/terrain.json`:
```
"compiledVisibleRadius": 2
```
Radius N renders a `(2N+1) x (2N+1)` tile window (e.g., 2 => 5x5).

LOD and skirts:
```
"compiledLod1Distance": 3000.0,
"compiledSkirtDepth": 180.0
```

## Terrain visuals (runtime)
The terrain renderer applies a cheap shader pass to reduce the "paint bucket" look:
- **Noise tint:** low-frequency variation per pixel.
- **Altitude tint:** valleys slightly warmer/brighter; higher terrain cooler/darker.
- **Slope darkening:** steep slopes darken to soften landuse edges.
- **Distance haze:** mixes toward sky color with range.

Optional config (defaults are safe):
```
"terrainVisuals": {
  "heightMin": 0.0,
  "heightMax": 1500.0,
  "noiseScale": 0.002,
  "noiseStrength": 0.3,
  "slopeStart": 0.3,
  "slopeEnd": 0.7,
  "slopeDarken": 0.3,
  "fogDistance": 12000.0,
  "desaturate": 0.2,
  "tint": [0.45, 0.52, 0.33],
  "tintStrength": 0.15,
  "distanceDesatStart": 3000.0,
  "distanceDesatEnd": 12000.0,
  "distanceDesatStrength": 0.35,
  "distanceContrastLoss": 0.25,
  "fogSunScale": 0.35
}
```

Optional compiler smoothing (mask edges):
```
--mask-smooth 1
```

## Terrain textures (runtime)
Textures are blended using masks (water/urban/forest/grass) plus slope-based rock and procedural dirt.
Extra controls help push a more "sim" look without ortho:
- **Macro variation:** large-scale patchiness to avoid flat repeats.
- **Micro variation:** high-frequency contrast to reduce blur.
- **Grass/forest/urban tint pairs:** mix between lush/dry and cool/warm looks.
- **Water detail:** procedural noise to break up flat water color.

Example config:
```
"terrainTextures": {
  "enabled": true,
  "grass": "../textures/terrain/sparse_grass_diff_2k.jpg",
  "forest": "../textures/terrain/forest_ground_04_diff_2k.png",
  "rock": "../textures/terrain/rock_05_diff_2k.png",
  "dirt": "../textures/terrain/dirt_diff_2k.jpg",
  "urban": "../textures/terrain/asphalt_02_diff_2k.png",
  "texScale": 0.04,
  "detailScale": 0.18,
  "detailStrength": 0.26,
  "macroScale": 0.0008,
  "macroStrength": 0.32,
  "grassTintA": [0.78, 0.98, 0.65],
  "grassTintB": [0.5, 0.7, 0.45],
  "grassTintStrength": 0.45,
  "forestTintA": [0.62, 0.78, 0.55],
  "forestTintB": [0.45, 0.6, 0.4],
  "forestTintStrength": 0.25,
  "urbanTintA": [0.95, 0.95, 0.96],
  "urbanTintB": [0.7, 0.72, 0.75],
  "urbanTintStrength": 0.25,
  "microScale": 0.3,
  "microStrength": 0.18,
  "waterDetailScale": 0.12,
  "waterDetailStrength": 0.3,
  "waterColor": [0.14, 0.32, 0.55]
}
```

## Trees (runtime, optional)
Trees are generated per compiled tile at runtime (simple trunk + cone canopy). They
use the existing mask weights: avoid water + urban, favor forest. No recompile needed.

Config:
```
"terrainTrees": {
  "enabled": true,
  "densityPerSqKm": 80.0,
  "minHeight": 4.0,
  "maxHeight": 10.0,
  "minRadius": 0.8,
  "maxRadius": 2.2,
  "maxSlope": 0.7,
  "maxDistance": 5000.0,
  "seed": 1337
}
```

## How to compile (Stage 1/2)
Example with DEM + OSM:
```
build/terrainc \
  --heightmap assets/terrain/sources/output_USGS10m_u16.png \
  --height-min -103.81412 --height-max 1335.2056 \
  --tile-size 2000 --grid 129 \
  --osm assets/terrain/sources/norcal-251226.osm.pbf \
  --mask-res 128 \
  --xmin -123.307174 --ymin 37.018272 --xmax -120.756699 --ymax 38.310244 \
  --out assets/terrain/compiled
```

DEM-only (no masks):
```
build/terrainc \
  --heightmap assets/terrain/sources/output_USGS10m_u16.png \
  --height-min -103.81412 --height-max 1335.2056 \
  --tile-size 2000 --grid 129 \
  --xmin -123.307174 --ymin 37.018272 --xmax -120.756699 --ymax 38.310244 \
  --out assets/terrain/compiled
```

## How to run
- Ensure `assets/config/terrain.json` points to the compiled manifest.
- Run the sim; the runtime will stream tiles automatically.

## Pros
- Deterministic, offline results.
- Runtime is simple and fast (small working set).
- No runtime GIS dependencies.
- Easy to debug (tile IDs, masks, logs).

## Cons
- Compile step can be slow for large bboxes.
- Landuse masks are coarse and incomplete (OSM coverage varies).
- No LOD/quadtrees yet (large areas increase tile count).
- Colorization is debug-level, not final visuals.

## Common failure modes
- **Empty landuse**: OSM may store features as relations; ensure osmium filters include relations.
- **Blocky masks**: `maskResolution` is low; raise to 256 or add smoothing.
- **Long compile times**: large bbox + many polygons; shrink bbox or optimize.
- **DEM won't load**: convert GeoTIFF to 16-bit PNG (stb_image limitation).

## Known limitations (current)
- No texture/imagery integration.
- No terrain material system.
- Heightmap input assumes local ENU projection derived from bbox.
- LOD swap is a hard cut (no geomorphing yet).
