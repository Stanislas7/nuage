# Assets organization

The `assets/` directory groups scenery-specific data under `assets/scenery/<name>/` so raw downloads, previews, and quadtree tiles live together.

## Directory layout
- `assets/scenery/<name>/imagery/`
  - `albedo_utm.tif` (projected source imagery for tiling)
  - `albedo_texture.jpg` (runtime-friendly preview texture)
  - `albedo_preview.jpg` (smaller preview for docs/QA)
  - `s2_visual/` and `s2_visual_mosaic.vrt` (Sentinel-2 downloads/cache)
  - `tiles/albedo/L00/` ... `Lxx/` (quadtree-ready imagery tiles)
- `assets/scenery/<name>/heights/`
  - `dem_utm.tif` (projected source DEM)
  - `dem_utm_uint16.tif` (scaled UInt16 DEM for tiling)
  - `dem_meta.json` (height range + size metadata)
  - `height_preview.png` (runtime-friendly preview heightmap)
  - `tiles/height/L00/` ... `Lxx/` (quadtree-ready height tiles)
- `assets/scenery/<name>/manifest.json` (tile pyramid metadata)
- `assets/scenery/<name>/terrain_preview.json` (preview config for the current renderer)
- `assets/config/terrain.json` remains the single active terrain config; swap it to point at the desired preview config.

## Scenery pipeline (automated)
`scripts/build_scenery.py` orchestrates the downloads, previews, and tile pyramid build in one step.

Example:
```bash
python3 scripts/build_scenery.py \
  --name sfo \
  --xmin -122.60 --ymin 37.57 --xmax -122.20 --ymax 37.92 \
  --start 2024-05-01T00:00:00Z --end 2024-10-31T23:59:59Z \
  --imagery-tr 10 --dem-tr 30 --tile-size 512 --levels 5
```

If you want the preview assets activated automatically, add `--activate` to update `assets/config/terrain.json`.

## Current sceneries
- **sfo** (legacy, single-texture runtime)
  - Heightmap: `assets/scenery/sfo/heights/sfo_dem.png`
  - Runtime texture: `assets/scenery/sfo/imagery/albedo_texture.jpg`
- **keywest**
  - Heightmap: `assets/scenery/keywest/heights/keywest_dem.png`
  - Imagery: (none yet; add JPEG/TIFFs under `imagery/`)

## Working with imagery
Run `scripts/get_s2_albedo.py` directly when you want full control over imagery search inputs. The automated pipeline above calls it for you and keeps the raw outputs under the scenery folder.

## Adding a new scenery
1. Run `scripts/build_scenery.py` with the new bbox + dates.
2. If you want to fly it immediately, set `assets/config/terrain.json` to the generated `assets/scenery/<name>/terrain_preview.json` (or use `--activate`).
3. Keep the generated `manifest.json` and `tiles/` folders for the upcoming streamed terrain renderer.
