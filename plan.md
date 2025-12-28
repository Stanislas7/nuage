# Nuage Architectural Refactoring Plan

This document outlines the roadmap to transition Nuage from a tightly coupled architecture to a modular, subsystem-based architecture inspired by FlightGear. The core mechanism for decoupling will be a global **Property Tree** (based on the existing `PropertyBus`).

## Goals
- **Decoupling:** Systems (Input, Physics, Graphics) should not directly reference each other.
- **Data-Driven:** Communication happens via a hierarchical property tree (e.g., `Input` writes to `/controls/flight/pitch`, `Physics` reads it).
- **Modularity:** It should be easy to swap out or disable subsystems without breaking the engine.

---

## Phase 1: Core Infrastructure
Establish the foundational classes for the new architecture.

- [ ] **Create `Subsystem` Interface (`src/core/subsystem.hpp`)**
    - Virtual methods: `init()`, `update(double dt)`, `shutdown()`, `getName()`.
- [ ] **Create `SubsystemManager` (`src/core/subsystem_manager.hpp`)**
    - Manages a list of subsystems.
    - Handles initialization order and the main update loop.
- [ ] **Global Property Access**
    - Ensure `PropertyBus` is accessible to all subsystems (likely via a Service Locator or Singleton pattern for now, to ease migration).

## Phase 2: Decouple Input System
Convert the existing `Input` class into a standalone subsystem that publishes data.

- [ ] **Refactor `Input` class**
    - Inherit from `Subsystem`.
    - Remove `FlightInput` struct and `flight()` getter.
- [ ] **Update Input Logic**
    - In `update()`, write values directly to property paths:
        - `/controls/flight/aileron` (Roll)
        - `/controls/flight/elevator` (Pitch)
        - `/controls/flight/rudder` (Yaw)
        - `/controls/engines/current/throttle`

## Phase 3: Decouple Aircraft (Physics)
Update the physics simulation to consume data from the property tree instead of direct function arguments.

- [ ] **Refactor `Aircraft` Update Loop**
    - Remove `Input& input` parameter from `update()`.
    - Read control surface positions from the `PropertyBus` at the start of the frame.
- [ ] **Refactor `FlightSession`**
    - Ensure `Aircraft` logic is called via the standard subsystem update path (or wrapped in a `PhysicsSystem`).

## Phase 4: Integration & Cleanup
Wire everything together in the main application loop.

- [ ] **Update `App` (`src/core/app.cpp`)**
    - Replace individual `m_input.update()`, etc., with `m_subsystemManager.update(dt)`.
    - Register `Input` and other systems with the manager on startup.
- [ ] **Update UI/HUD**
    - Modify HUD rendering to read aircraft state (airspeed, altitude) from the Property Tree instead of the `Aircraft` object directly.

## Phase 5: Future Steps (Post-Refactor)
- [ ] **Scripting Support:** Expose the Property Tree to Lua/Python.
- [ ] **Network Replication:** Sync properties over the network for multiplayer.
- [ ] **Configuration:** Load initial property values from JSON/XML files.
