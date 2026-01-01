# Nuage Flight Physics Deep Dive (JSBSim)

This document explains the long-lived physics architecture so the JSBSim integration can grow without requiring a rewrite of the docs every time.

## System rhythm and state
Nuage still steps physics in `App::updatePhysics` at a fixed timestep (`FIXED_DT = 1.0f / 120.0f`) and interpolates the results for rendering (`src/core/App.cpp`). Each `Aircraft::Instance` owns a `PropertyBus` (`core/property_bus.hpp`, `core/property_paths.hpp`) that stores the current orientation, position, velocity, control inputs, and derived state such as forces or atmosphere data. Before every physics tick the instance copies `m_state` into `m_prevState` so that the camera and rendering pipelines can interpolate smoothly between discrete updates.

## JSBSim path (Primary)
When an aircraft JSON contains a `jsbsim` block, the aircraft is driven by `JsbsimSystem`, which lazily initializes `JSBSim::FGFDMExec` once the initial state is loaded (`src/aircraft/systems/physics/jsbsim_system.cpp`). The system is wired into the aircraft through `Aircraft::Instance::addSystem`, so it reads the input keys and environment data that have already been written to the bus before the fixed-step update.

### Flow per timestep
1. **Inputs**: `Input` maps the keyboard to `FlightInput` axes (pitch, roll, yaw, throttle). The values are written to `input/*` properties before `JsbsimSystem` runs.
2. **Environment**: `EnvironmentSystem` updates `atmosphere/*` properties (wind, density) based on `Atmosphere::getWind`/`getAirDensity` and the aircraft's current position (`src/aircraft/systems/environment/environment_system.cpp`).
3. **JSBSim step**: `FGFDMExec::Run()` advances the FDM using the mapped inputs and wind values.
4. **Outputs**: The JSBSim state (position, velocity, body orientation, angular velocity, airspeed, ground speed, altitude) is translated back into Nuage properties (`position`, `orientation`, `velocity`, `physics/*`).

### Input mapping
Nuage keeps the key binding definitions under `assets/config/controls.json` and supports alternative physical layouts via `assets/config/layouts.json` when keys are remapped (`src/input/input.cpp`). The `Input` system normalizes named keycaps (e.g., `ArrowUp`, `KeypadAdd`) into GLFW scan codes and writes the combined pitch/roll/yaw/throttle commands onto the property bus so any system that cares about current controls can read them.

Parking brakes are modeled as a latched toggle on the bus (`controls/gear/parking-brake`). When engaged, Nuage forces the left/right brake commands to full until released, so JSBSim sees braking even when the brake pedal key is not held.

### HUD throttle indicator
The HUD renders a `PWR: xx%` indicator sourced from `input/throttle` so you can always see the throttle command currently being fed into JSBSim without reverse-engineering JSBSim's internal throttle plateaus. This is simply a text widget that reads the throttle property from the bus and converts it to percentage.

### Environment mapping
Nuage tracks wind/density via `Atmosphere` (`src/environment/atmosphere.cpp`), which currently implements a basic ISA density curve and uniform horizontal wind direction/speed. Those values are stored in the bus under `atmosphere/wind-*` and `atmosphere/density`, then converted into JSBSim's north/east/down convention inside `syncInputs()` before every JSBSim update (`src/aircraft/systems/physics/jsbsim_system.cpp`).

### Output mapping
JSBSim outputs are pulled from `prop->GetVel()`, `prop->GetPQR()`, `GetTb2l()`, and the `position/*`/`velocities/*` properties. `JsbsimSystem` converts and publishes:
- NED velocities into world `velocity/x,y,z` (`nedToWorld`).
- Body angular rates into Nuage body axes (`jsbBodyToNuage`).
- The JSBSim body-to-local matrix into a quaternion (`quatFromMatrix`) for `orientation`.
- Lat/lon/alt into local east/up/north coordinates using a small ECEF offset relative to the spawn location.
- `position/h-sl-ft` into `position/altitude-ft` (MSL).
- `position/h-agl-ft` into `position/altitude-agl-ft` (AGL, when terrain data is available).
- `velocities/vtrue-fps` into `velocities/airspeed-kt` (true airspeed).
- `velocities/vc-kts` into `velocities/airspeed-ias-kt` (calibrated/indicated airspeed).
- `velocities/vg-fps` into `velocities/groundspeed-kt`.

### Coordinate conventions
JSBSim uses body axes `X forward, Y right, Z down`. Nuage relies on `X right, Y up, Z forward`, so `JsbsimSystem` reorders axes both when pushing inputs (e.g., `fcs/aileron-cmd-norm` flips roll) and when reading outputs so the rest of the engine stays in Nuage's convention.

## Environment system
`EnvironmentSystem` runs alongside `JsbsimSystem` and pulls global atmospheric data from `Atmosphere`. It writes the density to `atmosphere/density` and derives a world wind vector from `Atmosphere::getWind`, keeping the property bus as the single source of truth for anything that needs current weather.

## Aircraft configuration (JSON + JSBSim XML)
Only a handful of fields are required when targeting JSBSim: the visual model, a spawn state, and the `jsbsim` block (`assets/config/aircraft/template.json`). Anything under `physics`, `engine`, `lift`, or `drag` is skipped when JSBSim is enabled because the JSBSim system owns the forces.

```
{
  "name": "My Aircraft",
  "model": {
    "name": "my_mesh",
    "path": "assets/models/my_mesh.obj",
    "color": [0.8, 0.8, 0.8],
    "scale": 1.0,
    "rotation": [0, 0, 0],
    "offset": [0, 0, 0]
  },
  "spawn": {
    "position": [0.0, 1000.0, 0.0],
    "airspeed": 60.0
  },
  "jsbsim": {
    "model": "c172p",
    "rootPath": "assets/jsbsim",
    "latitudeDeg": 0.0,
    "longitudeDeg": 0.0
  }
}
```

## Where JSBSim looks for aircraft
Nuage points JSBSim's root directory at `assets/jsbsim` by default, and the build copies the selected aircraft/engine/system XML files from `external/jsbsim` into the output `assets/jsbsim` tree. Within that tree JSBSim expects:

- `assets/jsbsim/aircraft/<model>/<model>.xml`
- `assets/jsbsim/engine/*.xml`
- `assets/jsbsim/systems/*.xml`

The CMake `JSBSIM_MODELS` list (see `CMakeLists.txt`) controls which aircraft folders are packaged, keeping the runtime bundle small unless you explicitly add more.

## Adding a new JSBSim aircraft
1. Copy the aircraft folder into `external/jsbsim/aircraft/<model>/` (matching JSBSim's layout).
2. Register `<model>` in the `JSBSIM_MODELS` list inside `CMakeLists.txt` so the build copies it into `assets/jsbsim/aircraft/<model>/`. You can also bypass `external/` by dropping the folder into `assets/jsbsim/aircraft/<model>/` and pointing `jsbsim.rootPath` at the new location.
3. Create an aircraft JSON (use `assets/config/aircraft/template_jsbsim.json`) that sets `jsbsim.model` to the folder name.
4. Build and run—the JSBSim system will now read your new XML definitions without touching any other game code.

If you need more telemetry (AOA, CL, gear state, …) you can expose additional JSBSim properties through `PropertyBus` in `JsbsimSystem::syncOutputs`.
