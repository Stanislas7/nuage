# Nuage Flight Simulator Architecture

## Core Architecture Overview

**Nuage** is a C++ flight simulator built with a **component-based architecture** centered around a **Property Bus** pattern. The simulator uses OpenGL for rendering, GLFW for window management, and follows a fixed-timestep physics approach.

---

## Property Bus System

The **PropertyBus** (`core/property_bus.hpp:10`) is the central data repository that enables loose coupling between systems:

### Design
- A simple `unordered_map<string, double>` that stores all simulation state
- All aircraft components communicate through property paths defined in `core/property_paths.hpp:4`
- Supports primitive types (doubles) and composite types (Vec3, Quat) via path conventions:
  - Vec3: `prefix/x`, `prefix/y`, `prefix/z`
  - Quat: `prefix/w`, `prefix/x`, `prefix/y`, `prefix/z`

### Property Path Structure
```
input/...           # Control inputs (pitch, roll, yaw, throttle)
position/...        # Aircraft position (x, y, z)
orientation/...     # Aircraft orientation (w, x, y, z) as quaternion
velocity/...        # Speed data (airspeed, vertical)
engine/...          # Engine state (thrust, n1, running, fuel_flow)
fuel/...            # Fuel quantity and capacity
atmosphere/...      # Environmental data (density, pressure, temperature, wind)
gear/...            # Landing gear extension
flaps/...           # Flap setting
```

### Why This Design?
- **Loose coupling**: Systems don't need direct references to each other
- **Data-driven**: Aircraft configs (JSON) define initial property values
- **Extensible**: New systems can read/write properties without modifying existing code

---

## Aircraft Component System

### Aircraft Instance
Each aircraft (`aircraft/aircraft.hpp:23`) is an `Instance` containing:
- **PropertyBus m_state**: All aircraft-specific state
- **vector<AircraftComponent> m_systems**: Simulation subsystems
- **Mesh/Shader**: Rendering resources

### Component Interface
All systems inherit from `AircraftComponent` (`aircraft/aircraft_component.hpp:8`):
```cpp
virtual const char* name() const = 0;
virtual void init(PropertyBus* state) = 0;
virtual void update(float dt) = 0;
```

### Component Update Flow
Each frame, `Aircraft::Instance::update` (`aircraft/aircraft_instance.cpp:113`):
1. Writes input properties to PropertyBus
2. Iterates through all systems and calls `update(dt)`
3. Each system reads/writes properties as needed

---

## Aircraft Systems

### 1. FlightDynamics (`aircraft/systems/flight_dynamics/`)

**Purpose**: Calculates aircraft motion based on inputs

**Key Behaviors**:
- **Throttle control** (`flight_dynamics.cpp:27`): Smoothly interpolates airspeed based on throttle input
- **Orientation update** (`flight_dynamics.cpp:38`):
  - Reads pitch/yaw/roll inputs
  - Creates local rotations based on current orientation axes
  - Combines: `yawRot * pitchRot * rollRot * orientation`
  - Normalizes to prevent drift
- **Position update** (`flight_dynamics.cpp:61`):
  - Calculates lift based on speed squared (`speedRatio * speedRatio`)
  - Computes net vertical acceleration (lift minus gravity)
  - Separates horizontal/vertical motion
  - Applies horizontal movement along forward vector
  - Applies vertical climb rate based on pitch and lift

**Physics Model**:
- Lift factor = `(speed / cruiseSpeed)²`, clamped to 1.5
- Net vertical accel = `(liftFactor - 1.0) * gravity`
- Climb rate = `speed * sin(pitch) + netVerticalAccel * 0.5`

---

### 2. EngineSystem (`aircraft/systems/engine/`)

**Purpose**: Simulates turbine engine behavior

**Key Behaviors**:
- **N1 spool-up** (`engine_system.cpp:32`):
  - Target N1 = idleN1 + (maxN1 - idleN1) * throttle
  - Exponential approach: `N1 = target + (current - target) * exp(-spoolRate * dt)`
- **Thrust calculation** (`engine_system.cpp:41`):
  - Thrust ratio = `(N1 - idleN1) / (maxN1 - idleN1)`
  - Adjusted by air density: `density / 1.225`
  - Final thrust = `maxThrust * thrustRatio * densityRatio`
- **Fuel flow** (`engine_system.cpp:46`):
  - Based on N1 ratio squared for realistic non-linear consumption
  - Range: `fuelFlowIdle + (fuelFlowMax - fuelFlowIdle) * n1Ratio²`

**Output Properties**:
- `engine/n1`: Current turbine RPM percentage
- `engine/thrust`: Current thrust in Newtons
- `engine/fuel_flow`: Fuel consumption in kg/s

---

### 3. FuelSystem (`aircraft/systems/fuel/`)

**Purpose**: Manages fuel state and engine cutoff

**Key Behaviors** (`fuel_system.cpp:20`):
- Consumes fuel: `quantity -= fuelFlow * dt`
- When fuel reaches 0, sets `engine/running = 0.0` (stops EngineSystem)

---

### 4. EnvironmentSystem (`aircraft/systems/environment/`)

**Purpose**: Couples aircraft to atmospheric conditions

**Key Behaviors** (`environment_system.cpp:17`):
- Reads current altitude from `position/y`
- Queries Atmosphere for air density at that altitude
- Writes to `atmosphere/density` (used by EngineSystem for thrust calculation)

---

## Main Loop Architecture

### App (`core/app.hpp:24`)

The main application orchestrates all systems:

**Initialization Flow** (`app.cpp:15`):
```
1. Create GLFW window with OpenGL 3.3 Core Profile
2. Initialize Input system
3. Load shaders (basic.vert, basic.frag)
4. Generate terrain mesh (20km x 20km)
5. Initialize Atmosphere
6. Initialize Aircraft manager
7. Initialize Camera
8. Initialize Scenery
9. Initialize UI
10. Setup scene and HUD
```

**Main Loop** (`app.cpp:44`):
```
while (running):
    1. Calculate deltaTime from last frame
    2. Update Input
    3. Update Physics (fixed timestep)
    4. Update Atmosphere
    5. Update Camera
    6. Update HUD
    7. Render
    8. Swap buffers / Poll events
```

---

### Physics Loop (`app.cpp:146`)

Uses a **fixed timestep accumulator** for deterministic physics:
```cpp
FIXED_DT = 1.0f / 120.0f  // 120 Hz physics

m_physicsAccumulator += deltaTime
while (m_physicsAccumulator >= FIXED_DT):
    aircraft.fixedUpdate(FIXED_DT, input)
    m_physicsAccumulator -= FIXED_DT
```

This ensures consistent physics regardless of frame rate.

---

## Input System

### Flow (`input/input.cpp:33`)

1. **Poll Keyboard** (`input.cpp:39`):
   - Tracks current and previous key states
   - Handles quit key (ESC)

2. **Map to Controls** (`input.cpp:51`):
   - Axis bindings map key pairs to control axes:
     - Pitch: W/S
     - Roll: D/A
     - Yaw: E/Q
   - Throttle: Smooth accumulation (SPACE to increase, SHIFT to decrease)

3. **FlightInput Output**:
   ```cpp
   struct FlightInput {
       float pitch, roll, yaw;  // -1 to 1
       float throttle;          // 0 to 1
       bool toggleGear, toggleFlaps, brake;
   };
   ```

---

## Camera System

### Camera Modes (`graphics/camera.hpp:11`)

- **Chase**: Follows behind aircraft with smoothing
- **Cockpit**: First-person view
- **Tower**: Fixed ground position
- **Orbit**: Rotates around aircraft

### Chase Camera (`camera.cpp`)

Uses **Lerp smoothing** for fluid camera movement:
- `positionSmoothing`: 5.0 (camera position lerp)
- `lookAtSmoothing`: 8.0 (camera target lerp)
- `forwardSmoothing`: 3.0 (aircraft forward vector for prediction)
- `predictionFactor`: 0.3 (predicts aircraft future position)

This prevents jitter and provides a cinematic feel.

---

## Graphics System

### AssetStore (`graphics/asset_store.hpp:11`)

Centralized resource manager:
- Loads and caches shaders and meshes
- Supports OBJ model loading via tiny_obj_loader
- Unloads all resources on shutdown

### Rendering Pipeline (`app.cpp:180`)

```
1. Clear buffers (color + depth)
2. Get view-projection matrix from camera
3. Render terrain (flat shader, no lighting)
4. Render scenery objects
5. Render aircraft instances
6. Render UI overlay
```

### Aircraft Rendering (`aircraft_instance.cpp:124`)

```cpp
Mat4 model = translate(position) * orientation.toMatrix()
MVP = viewProjection * model
shader->setMat4("uMVP", MVP)
shader->setVec3("uColor", aircraftColor)
mesh->draw()
```

---

## UI System

### HUD Overlay (`app.cpp:124`)

Displays flight information using the text renderer:
- **ALT**: Altitude (meters)
- **SPD**: Airspeed (m/s)
- **HDG**: Heading (degrees)
- **POS**: Position (X, Y, Z)

Updated each frame from aircraft PropertyBus state.

---

## Configuration System

### Aircraft Configuration (`assets/config/aircraft/cessna.json`)

JSON files define:
- **flightDynamics**: Speed limits, rotation rates, aerodynamic coefficients
- **engine**: Thrust curve, N1 range, spool rate, fuel consumption
- **fuel**: Capacity and initial quantity
- **model**: Mesh file and color

This allows creating multiple aircraft types without code changes.

---

## Data Flow Summary

### Initialization
```
main() -> App.init() -> Aircraft.spawnPlayer(configPath)
    -> Aircraft::Instance.init()
        -> Add systems (FlightDynamics, Engine, Fuel, Environment)
        -> Set initial property values from JSON
```

### Per-Frame Update
```
App.run():
    Input.update()
        -> Creates FlightInput from keyboard

    updatePhysics() [fixed timestep]:
        Aircraft::Instance.update(FlightInput)
            -> Write input properties to PropertyBus
            -> Call system.update(dt) for each:
                1. FlightDynamics: reads inputs, writes position/orientation
                2. EngineSystem: reads input/throttle, writes thrust/n1/fuel
                3. FuelSystem: reads fuel_flow, updates fuel quantity
                4. EnvironmentSystem: reads altitude, writes air density

    Camera.update() -> reads position/orientation from PropertyBus
    updateHUD() -> reads properties, updates text overlay
    render() -> draws everything using current state
```

---

## Key Design Decisions

### Why Property Bus?
- **Modularity**: Adding new systems only requires implementing `AircraftComponent`
- **Data-driven**: JSON configs define behavior without recompilation
- **Testing**: Systems can be tested in isolation with mock PropertyBuses

### Why Fixed Timestep Physics?
- **Determinism**: Same inputs always produce same outputs
- **Stability**: Small deltas prevent numerical instability in integrators
- **Frame rate independence**: Physics consistent at 30fps or 144fps

### Why Component-Based Aircraft?
- **Flexibility**: Different aircraft can have different system combinations
- **Configurable**: System parameters loaded from JSON
- **Maintainable**: Each system is self-contained with clear responsibility

---

This architecture enables Nuage to simulate realistic flight behavior while remaining easily extensible for new aircraft types, systems, and features.
