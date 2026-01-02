# Terrain Direction

Nuage uses a **compiled, real‑world terrain pipeline**. Procedural terrain generation has been removed; runtime only streams tiles produced by the offline compiler.

## Source Data
- **DEM heightmap** drives the mesh.
- **OSM / landclass masks** drive landuse (water, urban, forest, farmland, rock).
- **Runway data** snaps to the compiled mesh.

## Runtime Rendering
- Stream a fixed tile window around the camera.
- Two LOD levels + skirts to hide seams.
- Single-pass shader blends terrain textures using masks and slope.
- Macro/micro noise, tinting, and distance haze soften repetition.

## What To Tweak
- **Mask resolution** (recompile): higher `--mask-res` reduces blocky landclass tiles.
- **Texture tuning** (runtime config): `assets/config/terrain.json` controls scales, tints, and strengths.

## Why This Route
Compiled terrain keeps the system deterministic, fast, and GIS-accurate without runtime parsing. It also matches how flight sims handle non‑ortho terrain: real DEM + landclass + curated textures.
