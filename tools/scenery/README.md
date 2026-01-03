# Scenery Pipeline Tools

This folder contains the offline pipeline for building Nuage scenery packs.

## Build a Pack
1) Either place sources under `assets/scenery/sources/` or add `downloads` in the region config (see `assets/scenery/regions/bay_area.json`).
2) Ensure `gdalwarp`, `gdal_translate`, and `gdalbuildvrt` are in your PATH. If you use OSM, install `osmium`.
3) Build:
```
python3 tools/scenery/scenery_build.py --config assets/scenery/regions/bay_area.json --clean --download
```
`--download` pulls data from the URLs in the `downloads` block and can be large. `baseUrl` and `fileTemplate` can be arrays for fallbacks.

## Activate a Pack
```
python3 tools/scenery/scenery_sync.py activate --pack bay_area_v1
```

The active pack is referenced by `assets/config/terrain_flightgear.json`.
