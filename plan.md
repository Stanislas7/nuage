# Terrain Coordinates and Runway Flattening Plan

Focus first on the coordinate system so runway work has a stable foundation.

## Phase 1: Coordinate System (Now)
- [ ] **Define world origin policy**
  - Pick per-compiled-region origin (lat, lon, alt) and store in manifest.
  - Keep geodetic metadata for inputs and outputs.
- [ ] **Manifest schema update**
  - Add `originLatLonAlt` (and optionally `projection` or `enuBasis`) to `manifest.json`.
  - Document how runtime converts from geodetic to local ENU.
- [ ] **Compiler data flow**
  - Convert DEM sample grid + OSM/airport inputs into local ENU relative to origin.
  - Keep height units in meters; ensure axes and handedness are explicit.
- [ ] **Runtime integration**
  - Load origin from manifest and keep all terrain meshes in local ENU.
  - Provide a helper to convert lat/lon/alt to local ENU for gameplay placement.
- [ ] **Validation**
  - Spot-check a few known Bay Area landmarks for expected ENU positions.
  - Verify precision stability near runway scale (meters, not kilometers).

## Phase 2: Runway Flattening (Next)
- [ ] **Runway input format**
  - Ingest OurAirports CSV: endpoints, width, surface, heading.
  - Construct a rectangular runway polygon in local ENU.
- [ ] **Flattening algorithm**
  - Sample terrain at runway endpoints; fit a plane with zero cross-slope.
  - Apply a blend ring to ease into surrounding terrain.
- [ ] **Tile integration**
  - Apply flattening during compilation before mesh bake.
  - Optionally export a debug mask to visualize flatten areas.

## Phase 3: Full Airport Surfaces (Later)
- [ ] **Support taxiways/aprons**
  - Add OSM or curated polygon inputs.
  - Priority stack: runway > taxiway > apron > terrain.
