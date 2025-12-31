# Nuage Architecture

This document describes the modular, property-driven architecture of Nuage, inspired by professional flight simulators like FlightGear.

## Core Runtime & Subsystems
- **Subsystem Manager**: `core::App` utilizes a `SubsystemManager` to manage the lifecycle of major engine services. Each subsystem (`AssetStore`, `Input`, `UIManager`, `SimSubsystem`, `Audio`) implements a standard interface: `init()`, `update(double dt)`, and `shutdown()`. This keeps lifetime management centralized while allowing systems to be added or removed without modifying the core engine loop.
- **Subsystem Dependencies**: Subsystems can declare required dependencies by name. The manager validates presence and ordering during initialization to prevent hidden startup coupling.
- **Global Property Tree**: The "nervous system" of the engine is a global `PropertyBus`. Subsystems and components communicate by reading and writing to standardized property paths (e.g., `controls/flight/elevator`, `velocities/airspeed-kt`). This reduces direct dependencies for data flow (telemetry, controls), while core services still use explicit dependencies.
- **Main Loop**: `App::run` orchestrates the execution. It updates all subsystems, handles fixed-step physics accumulation (`1/120s`), and manages the rendering lifecycle with state interpolation for visual smoothness.

## Data Flow
1. **Input Subsystem**: Polls hardware (GLFW) and publishes normalized control values to the property tree under the `controls/` branch. It also issues simulation commands (e.g., `sim/commands/toggle-camera`).
2. **Simulation Subsystem**: Manages global simulation state, including `sim/time` and `sim/paused`.
3. **Physics (JSBSim)**: The `JsbsimSystem` (an `AircraftComponent`) reads control inputs from the property tree and computed values from the `EnvironmentSystem`. After running the FDM, it publishes the resulting telemetry (airspeed, altitude, orientation) back to the property bus.
4. **Audio Subsystem**: Reads control properties (e.g., `controls/engines/current/throttle`) to drive engine sound volume. On macOS it uses AVAudioEngine to loop the current engine WAV.
5. **UI & HUD**: UI overlays read the active aircraft's local property bus for per-aircraft telemetry. The UI remains data-driven, but the source of truth is the active `Aircraft::Instance` rather than the global bus.

## Aircraft System
- `Aircraft::Instance` is a container for a specific aircraft's state and components. It maintains a local `PropertyBus` for instance-specific data.
- **Components**: Functional logic (Physics, Engines, Systems) is implemented as `AircraftComponent` subclasses. They are initialized with access to both the instance's state and the global property tree.

## Graphics & Assets
- **AssetStore**: A subsystem-managed repository for shaders, textures, and models. Assets are loaded once and shared across the engine via `SubsystemManager`.
  Asset loading is data-driven via `assets/config/assets.json` to keep engine initialization decoupled from specific shader lists.
- **Camera**: A flexible camera system (Chase, Orbit) that tracks targets directly via `Aircraft::Instance` interpolation for tight coupling and predictable performance.
- **Terrain**: Managed by `TerrainRenderer`, which handles tile loading and LOD based on the current camera position.

## Configuration
- **Standardized Paths**: Nuage uses a hierarchical property naming convention (e.g., `controls/engines/current/throttle`).
- **JSON Configs**: Aircraft, controls, and simulator settings are defined in JSON files in `assets/config/`, allowing for rapid iteration without recompilation.
