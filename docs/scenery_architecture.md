# Nuage Scenery Architecture

Nuage uses an offline pipeline inspired by TerraGear/TerraSync, but outputs Nuage's compiled tiles and packs.

## Pipeline Stages
1) **Ingest sources**
   - DEM (USGS/SRTM) for elevation.
   - Landclass raster (ESA WorldCover) for category IDs.
   - OSM PBF (optional) for water/landuse/roads.
   - Airport/runway CSVs (OurAirports).
   - Optional: download inputs via `tools/scenery/scenery_build.py --download` using region `downloads` config.

2) **Normalize**
   - Clip sources to the region bbox.
   - Convert DEM to 16-bit PNG scaled to the height range.
   - Convert landclass to 16-bit PNG (raw IDs).

3) **Classify**
   - Map source landclass IDs to FlightGear landclass IDs using `assets/scenery/mappings/`.

4) **Compile**
   - Run `terrainc` to emit tiles + `manifest.json` with `maskType: "landclass"`.
   - Generate runway ENU JSON, then re-run `terrainc` to flatten runways and emit runtime `runways.json`.

5) **Package**
   - Pack outputs under `assets/scenery/packs/<pack>`.
   - Activate with `tools/scenery/scenery_sync.py` (sets `assets/scenery/active`).

## Runtime
Nuage streams the active pack and uses FlightGear materials/assets from `assets/terrain/fg`.

This keeps runtime independent while allowing the offline pipeline to evolve without touching the renderer.
