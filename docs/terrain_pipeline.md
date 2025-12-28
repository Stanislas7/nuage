# Terrain Pipeline (Offline Compiler + Runtime Streaming)

This document describes the current terrain system: what it does, how to run it,
and the tradeoffs. The pipeline is intentionally simple and offline-first.

## Why this exists
- Eliminate runtime GIS work (no GeoTIFF/OSM parsing in-engine).
- Make terrain deterministic and fast with a small working set.
- Keep the system solo-dev friendly and easy to debug.

## Big picture
**Offline:** `terrainc` compiles inputs into a tile pack on disk.  
**Runtime:** the engine streams a small tile window (3x3 or 5x5) and renders it.

```
DEM + OSM (offline) -> terrainc -> tiles/ + manifest.json -> runtime streamer
```

## Features (current)
- DEM-driven terrain mesh tiles.
- OSM water + landuse masks (debug coloring only).
- Terrain shading pass: low-frequency color variation, altitude tint, slope darkening, and distance haze.
- Small, fixed working set (radius tiles).
- No runtime GIS or reprojection.
- Deterministic results with explicit bounds and parameters.

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

Visible radius is configured in `assets/config/terrain.json`:
```
"compiledVisibleRadius": 2
```
Radius N renders a `(2N+1) x (2N+1)` tile window (e.g., 2 => 5x5).

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
