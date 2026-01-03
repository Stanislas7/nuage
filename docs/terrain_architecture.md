# Nuage Terrain Architecture

This document describes the current terrain pipeline, data layout, and runtime
integration for Nuage (FlightGear-style landclass materials, but fully local).

## Overview
Nuage terrain is compiled offline (similar to TerraGear) and streamed at runtime.
The runtime does not depend on FlightGear. All needed textures/materials are
vendored under `assets/terrain/fg`.

Pipeline summary:
1) Download/ingest sources (DEM + landclass + optional OSM).
2) Normalize and clip sources to the region bbox.
3) Convert landclass to raw 8-bit classes (PGM) for stable mask reads.
4) Compile tiles with `terrainc` (landclass mask in tiles).
5) Generate runways JSON with `ourairports_import`.
6) Re-run `terrainc` to flatten runways into tiles and emit pack outputs.
7) Activate the pack (symlink/copy) for runtime streaming.

## Data Layout
```
assets/scenery/
  regions/       # region config (bbox + sources + downloads)
  mappings/      # landclass mapping (ESA -> FG landclass IDs)
  sources/       # raw input rasters (VRTs + OSM PBF)
  work/          # intermediate outputs (clipped rasters, PGM)
  packs/         # compiled packs (manifest.json + tiles/)
  active/        # symlink/copy to selected pack

assets/terrain/fg/
  Materials/ Effects/ Shaders/ Terrain/ Runway/ Water/
```

## Runtime Configuration
The simulator uses `assets/config/terrain_flightgear.json` which points to:
- `compiledManifest`: `../scenery/active/manifest.json`
- `runways.json`: `../scenery/active/runways.json`
- `terrainMaterials.mode`: `flightgear`
- `terrainMaterials.fgRoot`: `../terrain/fg`

The renderer loads FG materials/landclass mappings and builds a texture array
plus a compact landclass LUT for fast lookup on GPU.

## Build Tools
From `nuage/`, build the tools:
```
cmake -S . -B build
cmake --build build --target terrainc ourairports_import
```

## Build a Scenery Pack (Bay Area preset)
```
python3 tools/scenery/scenery_build.py --config assets/scenery/regions/bay_area.json --clean --download
python3 tools/scenery/scenery_sync.py activate --pack bay_area_v1
```

Then run:
```
./run.sh
```

If you want to rebuild without re-downloading:
```
python3 tools/scenery/scenery_build.py --config assets/scenery/regions/bay_area.json
```

## Region Config (Key Fields)
`assets/scenery/regions/bay_area.json`:
- `bbox`: region bounds (lon/lat).
- `sources`: paths for DEM/landclass/OSM and airport CSVs.
- `downloads`: URLs for DEM, WorldCover, and Geofabrik OSM.
- `landclassMap`: maps ESA IDs to FG landclass IDs.
- `gridResolution`: mesh density per tile.
- `maskResolution`: landclass mask resolution per tile.
- `landclassMaxDim`: downsample cap before landclass conversion.

## Landclass Conversion
Landclass raster conversion uses:
1) `gdalwarp` to clip to bbox (EPSG:4326).
2) Optional downsample if the raster is too large.
3) `gdal_translate` to a grayscale GeoTIFF.
4) `gdal_translate` to PGM (non-paletted) to preserve raw class IDs.

This avoids palette expansion issues that can zero out mask data during load.

## Runways
Runways are derived from OurAirports CSVs and written to `runways.json`.
If the bbox excludes airports, the file may be missing; you can:
- expand the bbox to include an airport, or
- disable runways in `terrain_flightgear.json`.

## Troubleshooting
- **Red/white grid**: compiled tiles not loaded. Check `assets/scenery/active`
  points to the intended pack and `compiledDebugLog` is true for logs.
- **All water / checkerboard**: mask is all zeros. Rebuild with the PGM
  landclass output and confirm mask bytes are non-zero.
- **runways.json missing**: bbox excludes airports or runways were filtered out.
