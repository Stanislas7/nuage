# Nuage Simulation Architecture & Physics Documentation

## Core Architecture: The Property Bus

The core of the simulation relies on a **Property Bus** architecture. This design pattern decouples the various systems (aerodynamics, engine, input, rendering) from one another. Instead of objects calling methods on each other directly, they communicate by reading and writing values to a central shared state.

### How it works

The `PropertyBus` (`src/core/property_bus.hpp`) acts as a global "blackboard" or key-value store. It holds the entire state of the simulation frame.

*   **Data Types:** It supports `double`, `Vec3` (3D vectors), and `Quat` (Quaternions for rotation).
*   **Addressing:** Data is accessed via hierarchical string keys (e.g., `"sim/physics/velocity/x"` or `"sim/aircraft/engine/rpm"`).
*   **Flow:**
    1.  **Input System** reads hardware inputs and writes them to keys like `"sim/input/pitch"`.
    2.  **Physics Systems** (Lift, Drag, etc.) read the current state (velocity, orientation) and inputs from the bus.
    3.  **Physics Systems** calculate forces and write them back to specific force accumulators on the bus.
    4.  **Integrator** reads the total force, calculates acceleration, and updates the position/velocity on the bus.
    5.  **Renderer** reads the final position/orientation from the bus to draw the frame.

### Key Files
*   `src/core/property_bus.hpp`: The definition of the bus container.
*   `src/core/property_paths.hpp`: Defines the standard string keys used throughout the project to avoid typos (e.g., `Properties::Forces::LIFT_PREFIX`).

---

## Physics Engine (Updated)

The physics simulation is split into modular systems located in `src/aircraft/systems/physics/`. The previous implementation was "kinematic" for rotation, leading to a "floaty" feel. **The new implementation simulates full Rigid Body Dynamics including Rotational Physics.**

### The Physics Loop
The main loop runs inside `Aircraft::fixedUpdate`. It triggers the `PhysicsIntegrator` and various force systems.

1.  **Clear Forces & Torques:** The integrator resets all force and torque accumulators to zero.
2.  **Calculate Forces & Moments:**
    *   **Gravity:** $F = mg$ (Downwards).
    *   **Lift:** $L = C_L  q  S$ (Perpendicular to airflow).
    *   **Drag:** $D = C_D  q  S$ (Opposing airflow).
    *   **Thrust:** $T$ (Forward).
    *   **Control Moments:** Control surfaces generate **Torque** instead of direct rotation.
3.  **Integrate Linear Motion:** $F = ma ightarrow v ightarrow p$.
4.  **Integrate Rotational Motion:** $\tau = I\alpha \rightarrow \omega \rightarrow q$.

### Rotational Physics Implementation

The "floaty" behavior was fixed by implementing **Angular Momentum** and **Aerodynamic Damping**. The aircraft now behaves like a real physical object with mass distribution (Inertia).

#### 1. Inertia Tensor
Every aircraft has a defined Moment of Inertia vector $[I_{xx}, I_{yy}, I_{zz}]$ representing resistance to rotation around the Pitch, Yaw, and Roll axes.
*   **Heavy Aircraft:** High inertia values (hard to start rotating, hard to stop).
*   **Fighter Jets:** Low inertia values (snap maneuvers).

Defined in `physics.md` config or `cessna.json`/`plane.json`.

#### 2. Torque Generation (`OrientationSystem`)
Instead of directly modifying the orientation quaternion:
1.  **Input Mapping:** Joystick inputs are converted into **Control Surface Torques**.
    *   *Pitch Input* $\rightarrow$ Torque around X-axis.
    *   *Yaw Input* $\rightarrow$ Torque around Y-axis.
    *   *Roll Input* $\rightarrow$ Torque around Z-axis.
    *   Formula: $\tau_{input} = \text{Input} \times \text{Rate} \times \text{TorqueMultiplier} \times \text{DynamicPressureScale}$
2.  **Aerodynamic Damping:** To prevent the aircraft from spinning forever (like in space), we apply a counter-torque proportional to the current angular velocity.
    *   Formula: $\tau_{damping} = - \omega \times \text{DampingFactor} \times \text{DynamicPressureScale}$
3.  **Net Torque:** $\tau_{total} = \tau_{input} + \tau_{damping}$.

#### 3. Rotational Integration (`PhysicsIntegrator`)
The integrator updates the state using Euler integration for rotation:
1.  **Angular Acceleration:** $\alpha = \tau_{total} / I$ (Component-wise division by Inertia).
2.  **Angular Velocity:** $\omega_{new} = \omega_{old} + \alpha \cdot dt$.
3.  **Orientation:** The orientation Quaternion $q$ is updated by the angular velocity vector $\omega$.
    *   $q_{new} = q_{old} + 0.5 \cdot q_{old} \cdot \omega_{body} \cdot dt$.
    *   The result is normalized to ensure it remains a valid rotation.

### Tuning Parameters
The flight feel is controlled via JSON configuration (`assets/config/aircraft/*.json`):

*   `torqueMultiplier`: "Stiffness". How fast the plane accelerates towards the desired rotation rate. Higher = snappier.
*   `dampingFactor`: "Resistance". How fast the plane stops rotating when you let go. Higher = no overshoot, stops instantly.
*   `inertia`: The physical weight distribution.
    *   `[2000, 3500, 4500]` -> Light Fighter (Zero).
    *   `[1500000, ...]` -> Airliner.

### Why this fixes "Floatiness"
*   **Momentum:** If you roll hard and let go, the plane might coast a bit before stopping (depending on damping).
*   **Weight:** You feel the "heaviness" because the torque has to overcome the inertia.
*   **Speed Dependency:** Controls are scaled by airspeed. At low speeds (landing), controls feel "mushy" and less responsive because `controlScale` drops. At high speeds, they are firm.

---

### File Breakdown

#### `src/aircraft/systems/physics/orientation_system.cpp`
*   **Old:** Teleported rotation based on input.
*   **New:** Calculates physical **Torque** based on inputs and applies **Damping**. Writes to `Properties::Physics::TORQUE_PREFIX`.

#### `src/aircraft/systems/physics/physics_integrator.cpp`
*   **Old:** Only linear $F=ma$.
*   **New:** Handles both Linear ($F=ma$) and Rotational ($T=I\alpha$) integration. Maintains `Properties::Physics::ANGULAR_VELOCITY_PREFIX`.

#### `src/utils/config_loader.hpp` & `src/aircraft/aircraft_instance.cpp`
*   Updated to parse the new `inertia`, `torqueMultiplier`, and `dampingFactor` fields from JSON, allowing data-driven flight models.