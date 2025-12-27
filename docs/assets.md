# Assets organization

The `assets/` directory now groups scenery-specific data under `assets/scenery/<name>/` so imagery, heightmaps, and derived satellite tiles live together.

## Directory layout
- `assets/scenery/<name>/imagery/` — final diffuse textures for the renderer (`albedo.jpg`, `albedo_texture.jpg`, any downsampled previews or mosaics, and the Sentinel-2 cache under `s2_visual/`).
- `assets/scenery/<name>/heights/` — raw elevation tiles that supply gam e heightmaps (e.g. `sfo_dem.png` or `keywest_dem.png`).
- `assets/config/terrain.json` stays the single source for the in-game terrain path; swap in whichever `/scenery/<name>/heights/...` and `/imagery/...` files you want the renderer to use.

## Current sceneries
- **sfo**
  - Heightmap: `assets/scenery/sfo/heights/sfo_dem.png`
  - Satellite imagery / textures:
    - Full-res satellite: `assets/scenery/sfo/imagery/albedo.jpg` (too large for OpenGL alone; use `albedo_texture.jpg` for runtime).
    - Runtime texture: `assets/scenery/sfo/imagery/albedo_texture.jpg`
    - Preview/resample: `assets/scenery/sfo/imagery/albedo_preview.jpg`
    - Sentinel-2 downloads: `assets/scenery/sfo/imagery/s2_visual/` plus `s2_visual_mosaic.vrt`
- **keywest**
  - Heightmap: `assets/scenery/keywest/heights/keywest_dem.png`
  - Imagery: (none yet; add JPEG/TIFFs under `imagery/`)

## Working with imagery
Run `scripts/get_s2_albedo.py` with a bbox, dates, and the desired project CRS; it produces PNG/JPEG outputs under `assets/scenery/sfo/imagery` by default. Use the `--outdir` flag to direct output into other scenery folders whenever you add new regions.

## Adding a new scenery
1. Create `assets/scenery/<name>/imagery` and `.../heights`.
2. Place the heightmap (PNG/TIF) inside `heights/` and reference it from `assets/config/terrain.json` or a new terrain config you point `FlightConfig` at.
3. Add satellite textures under `imagery/`; keep the OpenGL-friendly version under a recognizable name (e.g., `albedo_texture.jpg`), and keep heavy intermediates (`albedo.jpg`, mosaics) there for later processing.
4. Update any scripts or configs that rely on the previous flat asset layout to point to the new folder paths.
