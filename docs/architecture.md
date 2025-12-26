# Nuage Architecture

This note documents the high-level structure so new features can slot into the existing runtime without constant re-explanation.

## Runtime loop and telemetry
- `core::App` owns the GLFW window, main loop, and subsystem wiring. Each frame it polls `Input`, accumulates a fixed-step physics tick (`FIXED_DT = 1/120`), updates the atmosphere and camera, refreshes HUD data, then renders the scene with interpolation to smooth between physics samples (`App::run`, `App::updatePhysics`, and the frame profile logging in `src/core/App.cpp`).
- The physics step is driven entirely through `PropertyBus` instances owned by each `Aircraft::Instance`. Systems read/write a shared set of named properties (`core/property_paths.hpp`) so every subsystem can exchange orientation, forces, control inputs, environment data, and telemetry without tight coupling.

## Input, controls, and the property bus
- `Input` owns raw GLFW state and reduces it into `FlightInput` values (pitch, roll, yaw, throttle, etc.). Bindings are defined in `assets/config/controls.json` and normalized against layouts from `assets/config/layouts.json` before the throttle/axis values are written back into the property bus on each physics tick (`src/input/input.cpp`, `assets/config/controls.json`).
- The `PropertyBus` stores doubles, vectors, and quaternions via string keys so systems such as `JsbsimSystem`, `EnvironmentSystem`, and HUD widgets can read what they need without owning the data.

## Aircraft systems and physics
- `Aircraft::Instance` is the per-aircraft owner of geometry, state, and systems (`src/aircraft/aircraft_instance.cpp`). Its init path loads a model/texture, spawns `EnvironmentSystem` and `JsbsimSystem`, seeds `position`, `velocity`, and `orientation`, and caches a copy of the previous bus state for interpolation.
- `EnvironmentSystem` samples the global `Atmosphere` for density and wind, writing `atmosphere/*` entries into the bus that JSBSim or other systems can reuse (`src/aircraft/systems/environment/environment_system.cpp`).
- `JsbsimSystem` exposes JSBSim as an `AircraftComponent`. It lazily initialises `FGFDMExec`, maps GL input to JSBSim commands/wind, runs `FGFDMExec::Run()`, and pushes JSBSim outputs (position, velocity, orientation matrix, angular velocity, true airspeed) back onto the bus in Nuage/world coordinates (`src/aircraft/systems/physics/jsbsim_system.cpp`).

## Graphics, UI, and camera
- `AssetStore` caches shaders, meshes, models, and textures so render code can request them on demand without reloading files (`src/graphics/asset_store.cpp`); models may split into multiple `Mesh` parts and track whether texturing is available.
- Rendering combines the camera's view (updated relative to the player `Aircraft::Instance` with interpolation) with the aircraft/model transform built from the bus position/orientation. The HUD is drawn by `UIManager`, which owns a font atlas, simple shader, and vertex buffer for text elements (`src/ui/ui_manager.cpp`).

## Build and asset pipeline
- `CMakeLists.txt` pulls in GLFW/OpenGL and embeds static JSBSim (disabling unused modules). The build copies `assets/`, JSBSim engines/systems, and any selected `JSBSIM_MODELS` (currently `c172p`) into the binary output directory so runtime loading just needs the `assets/jsbsim` tree.
- Assets/configs live under `assets/` (controls, layouts, scenery, simulator, terrain). Aircraft definitions reside in `assets/config/aircraft/` and only need to describe mesh spawn data plus the `jsbsim` block; physics/lift/engine sections are ignored when JSBSim is active because `JsbsimSystem` now owns the dynamics.
