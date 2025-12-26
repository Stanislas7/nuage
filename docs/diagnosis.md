# Codebase Diagnosis & Refactoring Plan

**Date:** December 26, 2025
**Scope:** Core Architecture, Property System, and Aircraft State Management

## 1. Critical Issue: The "Stringly Typed" PropertyBus

The current `PropertyBus` implementation (`src/core/property_bus.hpp`) acts as the central nervous system for the aircraft, but it suffers from significant architectural flaws.

### Problems
*   **Performance Bottleneck:** Every access involves `std::string` hashing and map lookups.
    *   *Example:* `m_state.getVec3("position")` triggers 3 separate map lookups for "position/x", "position/y", "position/z", plus string concatenation overhead.
*   **Type Unsafety:** All data is stored as `double`. Storing complex types (Vectors, Quaternions) requires manual serialization/deserialization.
*   **Fragility:** Usage relies on raw string literals defined in `property_paths.hpp`. A typo fails silently (returning `0.0`).
*   **No Reactivity:** Systems cannot subscribe to changes. They must poll the bus every frame.

### Proposed Solution: Hybrid State Model

We should split state into **Core State** (fast, struct-based) and **Generic Properties** (flexible, optimized).

#### TODOs
- [x] **Phase 1:** Introduce `PropertyId` (hashed strings) to eliminate runtime string hashing.
- [x] **Phase 2:** Create a concrete `AircraftState` struct for hot-path data.
- [x] **Phase 3:** Refactor `PropertyBus` to use templates or `std::variant`.

#### Example: Better API

```cpp
// 1. Core State (The "Hot Path")
struct AircraftState {
    Vec3 position;
    Quat orientation;
    Vec3 velocity;
    Vec3 angularVelocity;
    double airspeed;
};

// 2. Optimized Property System (The "Dynamic Path")
// Now supports direct storage of Vec3, Quat, double, etc.
m_bus.set(Properties::Atmosphere::WIND_PREFIX, myVec3); // Stored as a single Vec3 object
m_bus.set(Properties::Atmosphere::DENSITY, 1.225); // Type-safe double storage
```

---

## 2. Aircraft Instance & State Management

### Problems
*   **Inefficient Interpolation:** `m_prevState` is a full copy of the `PropertyBus` map. Copying an entire `std::unordered_map` every frame is extremely expensive.
*   **Data Flow:** Methods like `position()` reconstruct `Vec3` objects from the map on every call.

### Proposed Solution
Separate the simulation state from the property container.

#### TODOs
- [x] Store `current` and `previous` frames using the new `AircraftState` struct.
- [x] Only use the generic PropertyBus for aircraft-specific systems (e.g., "gear_extension_pct", "fuel_pump_state").

---

## 3. Rendering & Responsibility Separation

### Problems
`Aircraft::Instance::render` (`src/aircraft/aircraft_instance.cpp`) violates the Single Responsibility Principle.
*   It calculates physics interpolation.
*   It computes Model-View-Projection matrices.
*   It manages raw OpenGL state (`glUseProgram`, `glUniform...`).
*   It iterates over model parts.

### Proposed Solution
Extract rendering logic into a visual component.

#### TODOs
- [ ] Create `AircraftVisual` class.
- [ ] Move `render()` logic out of `Aircraft::Instance`.
- [ ] `Aircraft::Instance` should only provide the transform (Position/Rotation) to the visual system.

#### Example: Refactored Render Loop
```cpp
// In App::render()
// The visuals system just needs the "World Transform"
m_aircraftVisuals.draw(m_player->getTransform(), viewProjection);
```

---

## 4. Initialization & Configuration Boilerplate

### Problems
`Aircraft::Instance::init` is cluttered with repetitive JSON parsing logic.
```cpp
// Current boilerplate
if (!value.is_array() || value.size() != 3) return fallback;
return Vec3(value[0].get<float>(), ...);
```

### Proposed Solution
Centralize config parsing.

#### TODOs
- [ ] Enhance `src/utils/config_loader.hpp` or `src/utils/json.hpp`.
- [ ] Add explicit conversion operators for `Vec3` and `Quat` from JSON.

---

## Summary of Action Plan

1.  **Refactor `PropertyBus`**: Replace string keys with integer handles/hashes.
2.  **Struct-ify Core State**: Create `PhysicsState` struct to hold position/rotation/velocity.
3.  **Clean up Renderer**: Extract visual logic from the aircraft physics class.
4.  **Standardize Config**: Create robust JSON helpers.
