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

## Physics Engine

The physics simulation is split into modular systems located in `src/aircraft/systems/physics/`. The current implementation results in a "floaty" arcade-like feel primarily because **rotational physics are approximated** rather than simulated with moments and inertia.

### The Physics Loop
The main loop runs inside `Aircraft::fixedUpdate`. It triggers the `PhysicsIntegrator` and various force systems.

1.  **Clear Forces:** The integrator resets all force accumulators to zero.
2.  **Calculate Forces:** Each system (Gravity, Lift, Drag, Thrust) calculates its contribution and adds it to the total force vector on the bus.
3.  **Integrate:** The integrator takes the total force, divides by mass to get acceleration, and updates velocity and position.

### Why it "Floats" (The Problem)
The primary reason the aircraft feels like it floats rather than flies is the **Orientation System** (`src/aircraft/systems/physics/orientation_system.cpp`).

*   **Direct Rotation:** Instead of applying aerodynamic *torque* to the aircraft (which would accelerate angular velocity based on the moment of inertia), the system directly rotates the aircraft's orientation quaternion based on joystick input.
*   **No Angular Momentum:** There is no simulation of angular inertia. When you stop input, the rotation stops instantly (or behaves like a kinematic object) rather than preserving angular momentum or being damped by air resistance.
*   **Simple Integration:** The linear motion uses a basic Euler integrator, which is functional but simple.

### Detailed File Breakdown

#### 1. `src/aircraft/systems/physics/orientation_system.cpp` (The Culprit)
*   **Role:** Updates the aircraft's rotation.
*   **Behavior:** Reads `pitch`, `roll`, `yaw` inputs. It calculates a rotation delta based on input * `rate` * `dt` and applies it directly to the current orientation.
*   **Issue:** It bypasses physics. Real planes rotate because air hitting deflected control surfaces creates a *moment* (torque). This file makes it behave like a zero-mass camera controller.

#### 2. `src/aircraft/systems/physics/physics_integrator.cpp`
*   **Role:** The solver. It applies Newton's Second Law ($F=ma$).
*   **Behavior:**
    *   Sum of all forces / Mass = Acceleration.
    *   Velocity += Acceleration * dt.
    *   Position += Velocity * dt.
    *   Handles basic ground collision (keeps `y > minAltitude`).

#### 3. `src/aircraft/systems/physics/lift_system.cpp`
*   **Role:** Generates lift.
*   **Behavior:** Calculates Angle of Attack (AoA). Uses a linear lift coefficient model ($C_L = C_{L0} + C_{L	ext{	extalpha}} 	imes 	ext{	extalpha}$) clamped to min/max values.
*   **Output:** Adds a force vector perpendicular to the airflow.

#### 4. `src/aircraft/systems/physics/drag_system.cpp`
*   **Role:** Generates air resistance.
*   **Behavior:** Calculates Parasitic Drag (form drag) and Induced Drag (drag created by generating lift).
*   **Output:** Adds a force vector opposing the airflow.

#### 5. `src/aircraft/systems/physics/thrust_force.cpp`
*   **Role:** Propels the aircraft.
*   **Behavior:** Reads engine power/thrust. Calculates a forward force vector based on the aircraft's current orientation.

### Summary
To fix the "floaty" feeling, the `OrientationSystem` needs to be rewritten to:
1.  Calculate **Moments** (Torque) from control surfaces instead of direct rotation.
2.  Maintain an **Angular Velocity** vector in the Property Bus.
3.  Integrate Angular Velocity ($Torque / Inertia$) just like linear velocity.
