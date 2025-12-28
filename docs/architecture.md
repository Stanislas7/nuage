# Nuage Architecture

This document describes the modular, property-driven architecture of Nuage, inspired by professional flight simulators like FlightGear.

## Core Runtime & Subsystems
- **Subsystem Manager**: `core::App` utilizes a `SubsystemManager` to manage the lifecycle of all major engine components. Each subsystem (`Input`, `UIManager`, `SimSubsystem`) implements a standard interface: `init()`, `update(double dt)`, and `shutdown()`. This allows systems to be added or removed without modifying the core engine loop.
- **Global Property Tree**: The "nervous system" of the engine is a global `PropertyBus`. Subsystems communicate by reading and writing to standardized property paths (e.g., `controls/flight/elevator`, `velocities/airspeed-kt`). This eliminates direct dependencies between systems; for example, the Input system does not know the Physics system exists.
- **Main Loop**: `App::run` orchestrates the execution. It updates all subsystems, handles fixed-step physics accumulation (`1/120s`), and manages the rendering lifecycle with state interpolation for visual smoothness.

## Data Flow
1. **Input Subsystem**: Polls hardware (GLFW) and publishes normalized control values to the property tree under the `controls/` branch. It also issues simulation commands (e.g., `sim/commands/toggle-camera`).
2. **Simulation Subsystem**: Manages global simulation state, including `sim/time` and `sim/paused`.
3. **Physics (JSBSim)**: The `JsbsimSystem` (an `AircraftComponent`) reads control inputs from the property tree and computed values from the `EnvironmentSystem`. After running the FDM, it publishes the resulting telemetry (airspeed, altitude, orientation) back to the property bus.
4. **UI & HUD**: The `HudOverlay` and other UI elements read telemetry directly from the property tree. They have no knowledge of the underlying physics engine, making the UI purely data-driven.

## Aircraft System
- `Aircraft::Instance` is a container for a specific aircraft's state and components. It maintains a local `PropertyBus` for instance-specific data.
- **Components**: Functional logic (Physics, Engines, Systems) is implemented as `AircraftComponent` subclasses. They are initialized with access to both the instance's state and the global property tree.

## Graphics & Assets
- **AssetStore**: A central repository for shaders, textures, and models. It ensures assets are loaded once and shared across the engine.
- **Camera**: A flexible camera system (Chase, Orbit) that tracks targets by reading their interpolated position and orientation from the property tree.
- **Terrain**: Managed by `TerrainRenderer`, which handles tile loading and LOD based on the current camera position.

## Configuration
- **Standardized Paths**: Nuage uses a hierarchical property naming convention (e.g., `controls/engines/current/throttle`).
- **JSON Configs**: Aircraft, controls, and simulator settings are defined in JSON files in `assets/config/`, allowing for rapid iteration without recompilation.