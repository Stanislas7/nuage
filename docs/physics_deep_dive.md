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

1) **Add the JSBSim XML**
   - Drop the aircraft folder into `assets/jsbsim/aircraft/<model>/`.
   - The primary XML should be `<model>.xml`.

2) **Create a JSON config**
   - Add a new file under `assets/config/aircraft/` with a `jsbsim` block.
   - Set `jsbsim.model` to the JSBSim aircraft folder name.

3) **Use the model**
   - Spawn the aircraft as usual. The JSBSim system will run the flight dynamics.

## 6) Useful Runtime Properties

- `physics/air_speed` (meters/sec)
- `velocity/*` (world m/s)
- `orientation` (quaternion)
- `input/pitch`, `input/roll`, `input/yaw`, `input/throttle`

If you want to expose more JSBSim internals (AOA, CL, gear state), we can mirror those into the PropertyBus as needed.
