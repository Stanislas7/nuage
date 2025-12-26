# Nuage Flight Physics Deep Dive (JSBSim)

This document describes the current physics architecture after the JSBSim integration. Nuage now supports two physics paths:

- **JSBSim FDM** (preferred): full 6-DOF flight dynamics using JSBSim aircraft XML.
- **Legacy custom physics**: the older force-based model (still available for non-JSBSim aircraft).

## 1) Big Picture: Fixed Timestep + Property Bus

Nuage still runs a fixed physics timestep in `App::updatePhysics` and uses the `PropertyBus` for state. Systems read/write properties like `position`, `velocity`, `orientation`, `input/*`, and `physics/*`.

```
FIXED_DT = 1.0f / 120.0f
accumulator += frame_dt
while accumulator >= FIXED_DT:
    aircraft.fixedUpdate(FIXED_DT)
    accumulator -= FIXED_DT
```

## 2) JSBSim Path (Primary)

When `jsbsim.enabled` is true in an aircraft JSON, Nuage uses `JsbsimSystem` as the flight dynamics engine.

### Flow per timestep
1) **Inputs**: Nuage inputs are mapped to JSBSim control properties.
2) **Environment**: Wind is mapped to JSBSim wind components.
3) **JSBSim step**: `FGFDMExec::Run()` advances the FDM.
4) **Outputs**: Position, velocity, orientation, and airspeed are read back into the PropertyBus.

### Input mapping (current)
- `fcs/elevator-cmd-norm`  <- input pitch
- `fcs/aileron-cmd-norm`   <- input roll (sign adjusted to match Nuage convention)
- `fcs/rudder-cmd-norm`    <- input yaw
- `fcs/throttle-cmd-norm`  <- throttle

Throttle is bound to the `ArrowUp` / `ArrowDown` keys by default (with `PageUp`/`PageDown` and keypad `+`/`-` as fallbacks) defined in `assets/config/controls.json`. Those keycaps still map into the correct GLFW codes through the layout-aware loader so both AZERTY and QWERTY users get consistent input. Adjust the JSON array if you want different keycaps; `Input::loadBindingsFromConfig` remaps letter or named keys into the layout-aware physical key codes before the JSBSim throttle property is updated.

### UI throttle indicator
The HUD now renders a bottom-left `PWR: xx%` indicator sourced from `input/throttle`. It shows the currently commanded throttle percentage (clamped 0–100%) so you can see how much throttle your JSBSim aircraft is being asked to deliver without fighting the JSBSim throttle curve itself.

### Environment mapping
Nuage wind is in world axes (x=east, y=up, z=north). JSBSim expects NED (north/east/down) in ft/s. We convert and set:

- `atmosphere/wind-north-fps`
- `atmosphere/wind-east-fps`
- `atmosphere/wind-down-fps`

### Output mapping
JSBSim outputs are converted to Nuage coordinates:

- **Position**: JSBSim lat/lon/alt is converted into a local ENU position.
- **Velocity**: JSBSim NED velocity -> Nuage world velocity.
- **Orientation**: JSBSim body axes are mapped to Nuage body axes before converting to a quaternion.
- **Airspeed**: `velocities/vtrue-fps` is mapped to `physics/air_speed`.

### Coordinate conventions
- JSBSim body axes: **X forward, Y right, Z down**.
- Nuage body axes: **X right, Y up, Z forward**.

The adapter handles the axis conversion so the renderer and HUD behave consistently.

## 3) Legacy Custom Physics (Fallback)

If `jsbsim.enabled` is false or missing, Nuage uses the older force-based physics stack:

- EngineSystem -> ThrustForce
- LiftSystem / DragSystem
- StabilitySystem + OrientationSystem
- GravitySystem
- PhysicsIntegrator

This path is intact but not the preferred model for realism.

## 4) Aircraft Configuration (JSON + JSBSim XML)

### Minimal JSBSim aircraft JSON
You only need a model + spawn + jsbsim block. Physics/engine/lift/drag blocks are ignored when JSBSim is enabled.

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
    "enabled": true,
    "model": "c172p",
    "rootPath": "assets/jsbsim",
    "latitudeDeg": 0.0,
    "longitudeDeg": 0.0
  }
}
```

### Where JSBSim looks for aircraft
Nuage sets JSBSim root to `assets/jsbsim` by default. JSBSim then expects:

- `assets/jsbsim/aircraft/<model>/<model>.xml`
- `assets/jsbsim/engine/*.xml`
- `assets/jsbsim/systems/*.xml`

The build copies JSBSim aircraft/engine/systems into the build output under `assets/jsbsim/`.

## 5) Adding a New JSBSim Aircraft

Nuage only copies a **small subset** of JSBSim aircraft by default (see `CMakeLists.txt` and the `JSBSIM_MODELS` list). That keeps your `assets/jsbsim/aircraft/` folder lean. To add a new aircraft (e.g., a Piper):

1. **Provide the JSBSim XMLs**  
   - Copy or download the JSBSim aircraft folder into `external/jsbsim/aircraft/<model>/` (the same layout JSBSim expects).  
   - Update `JSBSIM_MODELS` in `CMakeLists.txt` to include `<model>` so the build copies it into `assets/jsbsim/aircraft/<model>/`.  
   - If you prefer not to touch `external/`, you can manually place the folder under `assets/jsbsim/aircraft/<model>/` and omit it from `JSBSIM_MODELS`; just ensure `jsbsim.rootPath` points to `assets/jsbsim` in your aircraft JSON.

2. **Create the aircraft JSON**  
   - Use `assets/config/aircraft/template_jsbsim.json` as a starting point.  
   - Set `jsbsim.model` to the folder name and optionally `jsbsim.rootPath` if you moved the XMLs elsewhere.  
   - You can ignore the old `physics`, `engine`, and `lift` sections—the JSBSim path handles those.

3. **Run the sim**  
   - The build copies the selected XMLs into your binary’s `assets/jsbsim/` tree at build time.  
   - The JSBSim system will read from `assets/jsbsim` and keep the rest of Nuage untouched.

If you ever want to add more JSBSim aircraft, just drop the XMLs (external or assets) and update `JSBSIM_MODELS`. No need to modify game code beyond the JSON config.

If you want to expose more JSBSim internals (AOA, CL, gear state), we can mirror those into the PropertyBus as needed.
