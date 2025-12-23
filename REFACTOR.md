# Flight Simulator Architecture Refactor

A modular, system-based architecture inspired by professional flight simulators.

## Table of Contents

1. [Core Architecture](#core-architecture)
2. [Module API Contract](#module-api-contract)
3. [Directory Structure](#directory-structure)
4. [Phase 1: Core Systems](#phase-1-core-systems)
5. [Phase 2: Simulation Systems](#phase-2-simulation-systems)
6. [Phase 3: Rendering Systems](#phase-3-rendering-systems)
7. [Phase 4: Entity & World](#phase-4-entity--world)
8. [Phase 5: Data-Driven Design](#phase-5-data-driven-design)
9. [API Examples](#api-examples)
10. [File-by-File Changes](#file-by-file-changes)
11. [Testing Strategy](#testing-strategy)
12. [Migration Path](#migration-path)

---

## Core Architecture

### The Big Idea

Every system is a **module** with a simple, consistent API:

```cpp
class Module {
public:
    virtual void init() = 0;
    virtual void update(float dt) = 0;  // Manipulate data, run logic
    virtual void render() = 0;           // Draw things, send to GPU
    virtual void shutdown() = 0;
};
```

The main loop just ticks each module in order:

```cpp
void Simulator::tick(float dt) {
    // Input first - gather what the player wants
    inputModule.update(dt);

    // Simulation - physics, AI, game logic
    aircraftModule.update(dt);
    terrainModule.update(dt);
    atmosphereModule.update(dt);

    // Cameras after simulation (they follow things)
    cameraModule.update(dt);

    // Rendering last
    terrainModule.render();
    aircraftModule.render();
    uiModule.render();
}
```

### Why This Works

- **Simple to understand**: Each module does one thing
- **Easy to add systems**: Drop in a new module, add to tick order
- **Clear data flow**: Update modifies state, Render reads state
- **Testable**: Modules can run without rendering
- **Maintainable**: Change one system without touching others

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         Simulator                                │
│                                                                  │
│  tick(dt) {                                                      │
│      inputModule.update(dt);                                     │
│      aircraftModule.update(dt);                                  │
│      terrainModule.update(dt);                                   │
│      cameraModule.update(dt);                                    │
│                                                                  │
│      terrainModule.render();                                     │
│      aircraftModule.render();                                    │
│  }                                                               │
└─────────────────────────────────────────────────────────────────┘
         │              │              │              │
         ▼              ▼              ▼              ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│InputModule  │ │AircraftModule│ │TerrainModule│ │CameraModule │
│─────────────│ │─────────────│ │─────────────│ │─────────────│
│ update()    │ │ update()    │ │ update()    │ │ update()    │
│ render()    │ │ render()    │ │ render()    │ │ render()    │
│             │ │             │ │             │ │             │
│ state:      │ │ state:      │ │ state:      │ │ state:      │
│ - keys      │ │ - aircraft[]│ │ - heightmap │ │ - position  │
│ - controls  │ │ - physics   │ │ - chunks    │ │ - matrices  │
└─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
                      │                               │
                      ▼                               ▼
              ┌─────────────┐                 ┌─────────────┐
              │FlightModel  │                 │RenderContext│
              │(pure math)  │                 │(view/proj)  │
              └─────────────┘                 └─────────────┘

Shared Resources (passed to modules that need them):
┌─────────────────────────────────────────────────────────────────┐
│ AssetStore: shaders, meshes, textures, configs                  │
│ World: entities, spatial queries                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Module API Contract

### Base Module Interface

**File: `include/core/module.hpp`**
```cpp
#pragma once

namespace flightsim {

class Simulator;

class Module {
public:
    virtual ~Module() = default;

    // Called once at startup
    virtual void init(Simulator* sim) { m_sim = sim; }

    // Called every tick - modify state, run logic
    // Not all modules need update (e.g., pure render helpers)
    virtual void update(float dt) {}

    // Called every tick after all updates - draw to screen
    // Not all modules need render (e.g., input, audio)
    virtual void render() {}

    // Called once at shutdown
    virtual void shutdown() {}

    // Optional: for modules that need variable-rate updates
    virtual void fixedUpdate(float fixedDt) {}

    // Module identification
    virtual const char* name() const = 0;

protected:
    Simulator* m_sim = nullptr;
};

}
```

### Module Communication

Modules communicate through:

1. **Shared state** (read-only during render)
2. **The Simulator** (access other modules)
3. **Events** (optional, for decoupled communication)

```cpp
// Accessing another module
void AircraftModule::update(float dt) {
    // Get input from InputModule
    const auto& controls = m_sim->input().controls();

    // Update aircraft with those controls
    for (auto& aircraft : m_aircraft) {
        aircraft.update(dt, controls);
    }
}

void AircraftModule::render() {
    // Get camera matrices from CameraModule
    const auto& ctx = m_sim->camera().renderContext();

    for (const auto& aircraft : m_aircraft) {
        drawAircraft(aircraft, ctx);
    }
}
```

---

## Directory Structure

```
sim/
├── assets/                          # Runtime data (loaded at runtime)
│   ├── config/
│   │   ├── simulator.json          # Window, graphics settings
│   │   ├── aircraft/
│   │   │   ├── cessna.json         # Aircraft specs
│   │   │   └── f16.json
│   │   ├── terrain.json            # Terrain generation params
│   │   └── controls.json           # Key bindings
│   └── shaders/
│       ├── basic.vert
│       ├── basic.frag
│       ├── terrain.vert
│       └── terrain.frag
│
├── include/
│   ├── core/                        # Engine foundation
│   │   ├── simulator.hpp           # Main class, owns all modules
│   │   ├── module.hpp              # Base module interface
│   │   ├── types.hpp               # Handles, IDs, common types
│   │   └── config.hpp              # Configuration loading
│   │
│   ├── modules/                     # All game systems
│   │   ├── input_module.hpp        # Keyboard input
│   │   ├── aircraft_module.hpp     # Aircraft spawning & management
│   │   ├── terrain_module.hpp      # Terrain generation & rendering
│   │   ├── camera_module.hpp       # Camera logic & matrices
│   │   └── render_module.hpp       # Core rendering (clear, swap)
│   │
│   ├── simulation/                  # Pure simulation (NO OpenGL)
│   │   ├── flight_model.hpp        # Flight dynamics math
│   │   ├── flight_params.hpp       # Aircraft configuration data
│   │   ├── aircraft_state.hpp      # Aircraft runtime state
│   │   └── physics.hpp             # Physics helpers
│   │
│   ├── graphics/                    # OpenGL wrappers
│   │   ├── shader.hpp
│   │   ├── mesh.hpp
│   │   ├── render_context.hpp      # View/proj matrices, lights
│   │   └── mesh_builder.hpp        # Procedural mesh creation
│   │
│   ├── world/                       # Entity management
│   │   ├── entity.hpp              # Base entity
│   │   ├── transform.hpp           # Position, rotation, scale
│   │   ├── aircraft.hpp            # Aircraft entity
│   │   └── terrain.hpp             # Terrain entity
│   │
│   ├── assets/                      # Asset loading
│   │   ├── asset_store.hpp         # Central registry
│   │   ├── shader_loader.hpp
│   │   └── config_loader.hpp
│   │
│   └── math/                        # Math library
│       ├── vec3.hpp
│       ├── mat4.hpp
│       ├── quat.hpp                # Quaternions
│       └── utils.hpp               # lerp, clamp, etc.
│
├── src/
│   ├── core/
│   │   ├── simulator.cpp
│   │   └── config.cpp
│   │
│   ├── modules/
│   │   ├── input_module.cpp
│   │   ├── aircraft_module.cpp
│   │   ├── terrain_module.cpp
│   │   ├── camera_module.cpp
│   │   └── render_module.cpp
│   │
│   ├── simulation/
│   │   └── flight_model.cpp
│   │
│   ├── graphics/
│   │   ├── shader.cpp
│   │   ├── mesh.cpp
│   │   └── mesh_builder.cpp
│   │
│   ├── assets/
│   │   ├── asset_store.cpp
│   │   ├── shader_loader.cpp
│   │   └── config_loader.cpp
│   │
│   ├── glad.c
│   └── main.cpp                     # Minimal entry point
│
├── tests/                           # Unit tests (no GL)
│   ├── test_math.cpp
│   ├── test_flight_model.cpp
│   └── test_config.cpp
│
├── CMakeLists.txt
└── REFACTOR.md
```

---

## Phase 1: Core Systems

### 1.1 Simulator (Main Class)

Owns all modules, runs the tick loop.

**File: `include/core/simulator.hpp`**
```cpp
#pragma once

#include "module.hpp"
#include "modules/input_module.hpp"
#include "modules/aircraft_module.hpp"
#include "modules/terrain_module.hpp"
#include "modules/camera_module.hpp"
#include "modules/render_module.hpp"
#include "assets/asset_store.hpp"
#include <memory>
#include <vector>

struct GLFWwindow;

namespace flightsim {

struct SimulatorConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    const char* title = "Flight Simulator";
    bool vsync = true;
};

class Simulator {
public:
    bool init(const SimulatorConfig& config = {});
    void run();
    void shutdown();

    // Module access (typed)
    InputModule& input() { return m_input; }
    AircraftModule& aircraft() { return m_aircraft; }
    TerrainModule& terrain() { return m_terrain; }
    CameraModule& camera() { return m_camera; }
    RenderModule& render() { return m_render; }

    // Shared resources
    AssetStore& assets() { return m_assets; }

    // State
    float time() const { return m_time; }
    float deltaTime() const { return m_deltaTime; }
    GLFWwindow* window() const { return m_window; }
    bool shouldClose() const;

    // Request shutdown
    void quit() { m_shouldQuit = true; }

private:
    void tick(float dt);
    void initModules();
    void shutdownModules();

    GLFWwindow* m_window = nullptr;

    // Core modules (order matters for update/render)
    InputModule m_input;
    AircraftModule m_aircraft;
    TerrainModule m_terrain;
    CameraModule m_camera;
    RenderModule m_render;

    // Shared resources
    AssetStore m_assets;

    // Timing
    float m_time = 0;
    float m_deltaTime = 0;
    float m_lastFrameTime = 0;
    bool m_shouldQuit = false;
};

}
```

**File: `src/core/simulator.cpp`**
```cpp
#include "core/simulator.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace flightsim {

bool Simulator::init(const SimulatorConfig& config) {
    // Init GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    m_window = glfwCreateWindow(config.windowWidth, config.windowHeight,
                                 config.title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    if (config.vsync) glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);

    // Init all modules
    initModules();

    m_lastFrameTime = glfwGetTime();
    return true;
}

void Simulator::initModules() {
    m_input.init(this);
    m_aircraft.init(this);
    m_terrain.init(this);
    m_camera.init(this);
    m_render.init(this);
}

void Simulator::run() {
    while (!shouldClose()) {
        float currentTime = glfwGetTime();
        m_deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
        m_time = currentTime;

        tick(m_deltaTime);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Simulator::tick(float dt) {
    // UPDATE PHASE - modify state
    m_input.update(dt);
    m_aircraft.update(dt);
    m_terrain.update(dt);
    m_camera.update(dt);

    // RENDER PHASE - draw to screen
    m_render.render();      // Clear, setup state
    m_terrain.render();
    m_aircraft.render();
}

bool Simulator::shouldClose() const {
    return m_shouldQuit || glfwWindowShouldClose(m_window);
}

void Simulator::shutdown() {
    shutdownModules();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Simulator::shutdownModules() {
    m_render.shutdown();
    m_camera.shutdown();
    m_terrain.shutdown();
    m_aircraft.shutdown();
    m_input.shutdown();
}

}
```

### 1.2 New main.cpp

Minimal entry point.

**File: `src/main.cpp`**
```cpp
#include "core/simulator.hpp"

int main() {
    flightsim::Simulator sim;

    if (!sim.init({
        .windowWidth = 1920,
        .windowHeight = 1080,
        .title = "Flight Simulator"
    })) {
        return -1;
    }

    sim.run();
    sim.shutdown();

    return 0;
}
```

### 1.3 Type Definitions

**File: `include/core/types.hpp`**
```cpp
#pragma once

#include <cstdint>
#include <string>

namespace flightsim {

// Entity identification
using EntityID = uint64_t;
constexpr EntityID INVALID_ENTITY = 0;

// Asset handles (type-safe references)
template<typename T>
struct Handle {
    uint32_t id = 0;
    bool valid() const { return id != 0; }
    bool operator==(const Handle& o) const { return id == o.id; }
};

class Shader;
class Mesh;

using ShaderHandle = Handle<Shader>;
using MeshHandle = Handle<Mesh>;

}
```

---

## Phase 2: Simulation Systems

### 2.1 Input Module

Polls keyboard, produces flight controls.

**File: `include/modules/input_module.hpp`**
```cpp
#pragma once

#include "core/module.hpp"
#include <unordered_map>
#include <string>

struct GLFWwindow;

namespace flightsim {

// Logical flight controls - what the pilot wants
struct FlightControls {
    float pitch = 0.0f;      // -1 (nose down) to +1 (nose up)
    float yaw = 0.0f;        // -1 (left) to +1 (right)
    float roll = 0.0f;       // -1 (left) to +1 (right)
    float throttle = 0.0f;   // 0 to 1
    bool brake = false;
};

class InputModule : public Module {
public:
    const char* name() const override { return "Input"; }

    void init(Simulator* sim) override;
    void update(float dt) override;

    // Access current controls
    const FlightControls& controls() const { return m_controls; }

    // Queries
    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;  // Just pressed this frame
    bool quitRequested() const { return m_quitRequested; }

    // Rebinding
    void loadBindings(const std::string& path);

private:
    void pollKeyboard();
    void mapToControls(float dt);

    GLFWwindow* m_window = nullptr;
    FlightControls m_controls;

    bool m_keys[512] = {};
    bool m_prevKeys[512] = {};
    bool m_quitRequested = false;

    float m_currentThrottle = 0.3f;  // Throttle accumulates

    // Key bindings (key code -> action name)
    struct AxisBinding {
        int positiveKey;
        int negativeKey;
    };
    std::unordered_map<std::string, AxisBinding> m_axisBindings;
    std::unordered_map<std::string, int> m_keyBindings;
};

}
```

**File: `src/modules/input_module.cpp`**
```cpp
#include "modules/input_module.hpp"
#include "core/simulator.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>

namespace flightsim {

void InputModule::init(Simulator* sim) {
    Module::init(sim);
    m_window = sim->window();

    // Default bindings
    m_axisBindings["pitch"] = {GLFW_KEY_W, GLFW_KEY_S};
    m_axisBindings["yaw"] = {GLFW_KEY_A, GLFW_KEY_D};
    m_axisBindings["roll"] = {GLFW_KEY_Q, GLFW_KEY_E};

    m_keyBindings["throttle_up"] = GLFW_KEY_SPACE;
    m_keyBindings["throttle_down"] = GLFW_KEY_LEFT_SHIFT;
    m_keyBindings["brake"] = GLFW_KEY_B;
    m_keyBindings["quit"] = GLFW_KEY_ESCAPE;
}

void InputModule::update(float dt) {
    pollKeyboard();
    mapToControls(dt);
}

void InputModule::pollKeyboard() {
    // Save previous state
    std::copy(std::begin(m_keys), std::end(m_keys), std::begin(m_prevKeys));

    // Poll current state
    for (int key = 0; key < 512; key++) {
        m_keys[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    // Check quit
    if (m_keys[m_keyBindings["quit"]]) {
        m_quitRequested = true;
    }
}

void InputModule::mapToControls(float dt) {
    auto mapAxis = [&](const std::string& name) -> float {
        auto it = m_axisBindings.find(name);
        if (it == m_axisBindings.end()) return 0.0f;
        float value = 0.0f;
        if (m_keys[it->second.positiveKey]) value += 1.0f;
        if (m_keys[it->second.negativeKey]) value -= 1.0f;
        return value;
    };

    m_controls.pitch = mapAxis("pitch");
    m_controls.yaw = mapAxis("yaw");
    m_controls.roll = mapAxis("roll");

    // Throttle accumulates over time
    if (m_keys[m_keyBindings["throttle_up"]]) {
        m_currentThrottle = std::min(1.0f, m_currentThrottle + dt * 0.5f);
    }
    if (m_keys[m_keyBindings["throttle_down"]]) {
        m_currentThrottle = std::max(0.0f, m_currentThrottle - dt * 0.5f);
    }
    m_controls.throttle = m_currentThrottle;

    m_controls.brake = m_keys[m_keyBindings["brake"]];
}

bool InputModule::isKeyDown(int key) const {
    return key < 512 && m_keys[key];
}

bool InputModule::isKeyPressed(int key) const {
    return key < 512 && m_keys[key] && !m_prevKeys[key];
}

}
```

### 2.2 Flight Model (Pure Math - NO OpenGL)

The core flight dynamics simulation. Zero dependencies on graphics.

**File: `include/simulation/aircraft_state.hpp`**
```cpp
#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace flightsim {

// Aircraft state - pure data, no behavior
struct AircraftState {
    Vec3 position{0, 100, 0};
    Quat orientation = Quat::identity();
    Vec3 velocity{0, 0, 20};          // m/s

    float throttle = 0.3f;            // 0 to 1
    float speed = 20.0f;              // m/s (computed from velocity)

    // Control surface positions (-1 to 1)
    float elevator = 0;
    float ailerons = 0;
    float rudder = 0;
};

}
```

**File: `include/simulation/flight_params.hpp`**
```cpp
#pragma once

#include <string>

namespace flightsim {

// Aircraft configuration - loaded from JSON
struct FlightParams {
    std::string name = "Default";

    // Speed limits
    float minSpeed = 20.0f;   // m/s (stall)
    float maxSpeed = 80.0f;   // m/s

    // Control rates (rad/s at full deflection)
    float pitchRate = 1.5f;
    float yawRate = 1.0f;
    float rollRate = 2.0f;

    // Limits
    float maxPitch = 1.2f;    // radians
    float minAltitude = 5.0f; // meters (ground)

    // Engine
    float throttleResponse = 0.5f;  // How fast throttle changes

    // Load from file
    static FlightParams load(const std::string& path);
};

}
```

**File: `include/simulation/flight_model.hpp`**
```cpp
#pragma once

#include "aircraft_state.hpp"
#include "flight_params.hpp"
#include "modules/input_module.hpp"  // For FlightControls

namespace flightsim {

// Pure flight dynamics - NO OpenGL, fully testable
class FlightModel {
public:
    void init(const FlightParams& params);
    void update(float dt, const FlightControls& controls);

    // State access
    const AircraftState& state() const { return m_state; }
    AircraftState& state() { return m_state; }

    // Convenience
    Vec3 position() const { return m_state.position; }
    Quat orientation() const { return m_state.orientation; }
    float speed() const { return m_state.speed; }
    float altitude() const { return m_state.position.y; }

    Vec3 forward() const;
    Vec3 up() const;
    Vec3 right() const;

    const FlightParams& params() const { return m_params; }

private:
    void updateThrottle(float dt, float throttleInput);
    void updateOrientation(float dt, const FlightControls& controls);
    void updatePosition(float dt);
    void enforceConstraints();

    FlightParams m_params;
    AircraftState m_state;
};

}
```

**File: `src/simulation/flight_model.cpp`**
```cpp
#include "simulation/flight_model.hpp"
#include <algorithm>
#include <cmath>

namespace flightsim {

void FlightModel::init(const FlightParams& params) {
    m_params = params;
    m_state = AircraftState{};
}

void FlightModel::update(float dt, const FlightControls& controls) {
    updateThrottle(dt, controls.throttle);
    updateOrientation(dt, controls);
    updatePosition(dt);
    enforceConstraints();
}

void FlightModel::updateThrottle(float dt, float throttleInput) {
    // Throttle directly from input (already accumulated in InputModule)
    m_state.throttle = std::clamp(throttleInput, 0.0f, 1.0f);

    // Speed based on throttle
    float targetSpeed = m_params.minSpeed +
        (m_params.maxSpeed - m_params.minSpeed) * m_state.throttle;

    // Smooth speed changes
    float speedDelta = (targetSpeed - m_state.speed) * m_params.throttleResponse * dt;
    m_state.speed = m_state.speed + speedDelta;
}

void FlightModel::updateOrientation(float dt, const FlightControls& controls) {
    // Apply control inputs as rotation rates
    float pitchDelta = controls.pitch * m_params.pitchRate * dt;
    float yawDelta = controls.yaw * m_params.yawRate * dt;
    float rollDelta = controls.roll * m_params.rollRate * dt;

    // Apply rotations in local space
    Quat pitchRot = Quat::fromAxisAngle(right(), pitchDelta);
    Quat yawRot = Quat::fromAxisAngle(Vec3{0, 1, 0}, yawDelta);
    Quat rollRot = Quat::fromAxisAngle(forward(), rollDelta);

    m_state.orientation = (yawRot * pitchRot * rollRot * m_state.orientation).normalized();
}

void FlightModel::updatePosition(float dt) {
    Vec3 velocity = forward() * m_state.speed;
    m_state.position = m_state.position + velocity * dt;
}

void FlightModel::enforceConstraints() {
    // Ground collision
    if (m_state.position.y < m_params.minAltitude) {
        m_state.position.y = m_params.minAltitude;
    }

    // Speed limits
    m_state.speed = std::clamp(m_state.speed, m_params.minSpeed, m_params.maxSpeed);
}

Vec3 FlightModel::forward() const {
    return m_state.orientation.rotate(Vec3{0, 0, 1});
}

Vec3 FlightModel::up() const {
    return m_state.orientation.rotate(Vec3{0, 1, 0});
}

Vec3 FlightModel::right() const {
    return m_state.orientation.rotate(Vec3{1, 0, 0});
}

}
```

### 2.3 Aircraft Module

Manages all aircraft in the simulation.

**File: `include/modules/aircraft_module.hpp`**
```cpp
#pragma once

#include "core/module.hpp"
#include "core/types.hpp"
#include "simulation/flight_model.hpp"
#include "graphics/mesh.hpp"
#include <vector>
#include <memory>

namespace flightsim {

// An aircraft instance
struct Aircraft {
    EntityID id = INVALID_ENTITY;
    std::string name;
    FlightModel model;
    MeshHandle mesh;
    ShaderHandle shader;
    bool isPlayer = false;
};

class AircraftModule : public Module {
public:
    const char* name() const override { return "Aircraft"; }

    void init(Simulator* sim) override;
    void update(float dt) override;
    void render() override;
    void shutdown() override;

    // Spawn aircraft
    Aircraft& spawn(const std::string& configPath);
    Aircraft& spawnPlayer(const std::string& configPath);

    // Access
    Aircraft* player() { return m_player; }
    Aircraft* find(EntityID id);
    const std::vector<std::unique_ptr<Aircraft>>& all() const { return m_aircraft; }

    // Destroy
    void destroy(EntityID id);
    void destroyAll();

private:
    void initDefaultMesh();

    std::vector<std::unique_ptr<Aircraft>> m_aircraft;
    Aircraft* m_player = nullptr;
    EntityID m_nextId = 1;

    // Default mesh for aircraft
    Mesh m_defaultMesh;
};

}
```

**File: `src/modules/aircraft_module.cpp`**
```cpp
#include "modules/aircraft_module.hpp"
#include "core/simulator.hpp"
#include "graphics/mesh_builder.hpp"
#include <glad/glad.h>

namespace flightsim {

void AircraftModule::init(Simulator* sim) {
    Module::init(sim);
    initDefaultMesh();
}

void AircraftModule::initDefaultMesh() {
    // Simple aircraft shape
    auto verts = MeshBuilder::aircraft({
        .fuselageLength = 4.0f,
        .wingspan = 6.0f,
        .bodyColor = {0.8f, 0.2f, 0.2f},
        .wingColor = {0.3f, 0.3f, 0.4f}
    });
    m_defaultMesh.init(verts);
}

Aircraft& AircraftModule::spawn(const std::string& configPath) {
    auto aircraft = std::make_unique<Aircraft>();
    aircraft->id = m_nextId++;
    aircraft->model.init(FlightParams::load(configPath));

    Aircraft& ref = *aircraft;
    m_aircraft.push_back(std::move(aircraft));
    return ref;
}

Aircraft& AircraftModule::spawnPlayer(const std::string& configPath) {
    Aircraft& a = spawn(configPath);
    a.isPlayer = true;
    a.name = "Player";
    m_player = &a;
    return a;
}

void AircraftModule::update(float dt) {
    const auto& controls = m_sim->input().controls();

    for (auto& aircraft : m_aircraft) {
        if (aircraft->isPlayer) {
            // Player uses input controls
            aircraft->model.update(dt, controls);
        } else {
            // AI aircraft - no controls for now
            aircraft->model.update(dt, FlightControls{});
        }
    }
}

void AircraftModule::render() {
    auto* shader = m_sim->assets().getShader("basic");
    if (!shader) return;

    shader->use();
    GLint mvpLoc = shader->getUniformLocation("uMVP");

    const auto& ctx = m_sim->camera().renderContext();

    for (const auto& aircraft : m_aircraft) {
        Mat4 model = Mat4::translate(aircraft->model.position())
                   * aircraft->model.orientation().toMat4()
                   * Mat4::scale(2.0f, 2.0f, 2.0f);

        Mat4 mvp = ctx.viewProjection * model;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp.data());

        m_defaultMesh.draw();
    }
}

void AircraftModule::shutdown() {
    destroyAll();
}

void AircraftModule::destroyAll() {
    m_aircraft.clear();
    m_player = nullptr;
}

Aircraft* AircraftModule::find(EntityID id) {
    for (auto& a : m_aircraft) {
        if (a->id == id) return a.get();
    }
    return nullptr;
}

}
```

---

## Phase 3: Rendering Systems

### 3.1 Camera Module

**File: `include/modules/camera_module.hpp`**
```cpp
#pragma once

#include "core/module.hpp"
#include "graphics/render_context.hpp"
#include "math/vec3.hpp"
#include "math/mat4.hpp"

namespace flightsim {

enum class CameraMode {
    Chase,      // Behind and above target
    Cockpit,    // First person
    Free        // Debug free-fly
};

class CameraModule : public Module {
public:
    const char* name() const override { return "Camera"; }

    void init(Simulator* sim) override;
    void update(float dt) override;

    // Access
    const RenderContext& renderContext() const { return m_context; }
    Vec3 position() const { return m_position; }

    // Camera settings
    void setMode(CameraMode mode) { m_mode = mode; }
    void setFov(float fov) { m_fov = fov; }
    void setAspect(float aspect) { m_aspect = aspect; }

    // Chase camera settings
    void setChaseDistance(float d) { m_chaseDistance = d; }
    void setChaseHeight(float h) { m_chaseHeight = h; }
    void setChaseSmoothing(float s) { m_chaseSmoothing = s; }

private:
    void updateChaseCamera(float dt);
    void updateFreeCamera(float dt);
    void buildMatrices();

    CameraMode m_mode = CameraMode::Chase;
    RenderContext m_context;

    Vec3 m_position{0, 100, -50};
    Vec3 m_target{0, 100, 0};

    // Projection
    float m_fov = 60.0f * 3.14159f / 180.0f;
    float m_aspect = 16.0f / 9.0f;
    float m_near = 0.1f;
    float m_far = 5000.0f;

    // Chase camera
    float m_chaseDistance = 25.0f;
    float m_chaseHeight = 10.0f;
    float m_chaseSmoothing = 5.0f;
};

}
```

**File: `src/modules/camera_module.cpp`**
```cpp
#include "modules/camera_module.hpp"
#include "core/simulator.hpp"

namespace flightsim {

void CameraModule::init(Simulator* sim) {
    Module::init(sim);
}

void CameraModule::update(float dt) {
    switch (m_mode) {
        case CameraMode::Chase:
            updateChaseCamera(dt);
            break;
        case CameraMode::Free:
            updateFreeCamera(dt);
            break;
        default:
            break;
    }

    buildMatrices();
}

void CameraModule::updateChaseCamera(float dt) {
    auto* player = m_sim->aircraft().player();
    if (!player) return;

    Vec3 targetPos = player->model.position();
    Vec3 targetForward = player->model.forward();

    // Desired camera position: behind and above
    Vec3 desiredPos = targetPos
                    - targetForward * m_chaseDistance
                    + Vec3{0, m_chaseHeight, 0};

    // Smooth interpolation
    float t = 1.0f - std::exp(-m_chaseSmoothing * dt);
    m_position = m_position + (desiredPos - m_position) * t;
    m_target = targetPos;
}

void CameraModule::updateFreeCamera(float dt) {
    // Free camera controlled by input (for debugging)
    // Could add WASD movement here
}

void CameraModule::buildMatrices() {
    m_context.view = Mat4::lookAt(m_position, m_target, Vec3{0, 1, 0});
    m_context.projection = Mat4::perspective(m_fov, m_aspect, m_near, m_far);
    m_context.viewProjection = m_context.projection * m_context.view;
    m_context.cameraPosition = m_position;
}

}
```

### 3.2 Render Context

**File: `include/graphics/render_context.hpp`**
```cpp
#pragma once

#include "math/mat4.hpp"
#include "math/vec3.hpp"

namespace flightsim {

// Passed to anything that needs to render
struct RenderContext {
    Mat4 view;
    Mat4 projection;
    Mat4 viewProjection;
    Vec3 cameraPosition;

    // Future: lighting, time, etc.
    Vec3 sunDirection{0.5f, -1.0f, 0.3f};
    float time = 0;
};

}
```

### 3.3 Render Module

**File: `include/modules/render_module.hpp`**
```cpp
#pragma once

#include "core/module.hpp"
#include "math/vec3.hpp"

namespace flightsim {

class RenderModule : public Module {
public:
    const char* name() const override { return "Render"; }

    void init(Simulator* sim) override;
    void render() override;

    void setClearColor(const Vec3& color) { m_clearColor = color; }
    void setWireframe(bool enabled) { m_wireframe = enabled; }

private:
    Vec3 m_clearColor{0.5f, 0.7f, 0.9f};
    bool m_wireframe = false;
};

}
```

**File: `src/modules/render_module.cpp`**
```cpp
#include "modules/render_module.hpp"
#include "core/simulator.hpp"
#include <glad/glad.h>

namespace flightsim {

void RenderModule::init(Simulator* sim) {
    Module::init(sim);
    glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, 1.0f);
}

void RenderModule::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

}
```

### 3.4 Terrain Module

**File: `include/modules/terrain_module.hpp`**
```cpp
#pragma once

#include "core/module.hpp"
#include "graphics/mesh.hpp"

namespace flightsim {

struct TerrainParams {
    int gridSize = 100;
    float scale = 10.0f;
    float heightScale = 1.0f;
};

class TerrainModule : public Module {
public:
    const char* name() const override { return "Terrain"; }

    void init(Simulator* sim) override;
    void render() override;
    void shutdown() override;

    // Configuration
    void generate(const TerrainParams& params);

    // Height queries (for physics/collision)
    float heightAt(float x, float z) const;

private:
    Mesh m_mesh;
    TerrainParams m_params;
    bool m_generated = false;
};

}
```

---

## Phase 4: Entity & World

### 4.1 Transform

**File: `include/world/transform.hpp`**
```cpp
#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "math/mat4.hpp"

namespace flightsim {

struct Transform {
    Vec3 position{0, 0, 0};
    Quat rotation = Quat::identity();
    Vec3 scale{1, 1, 1};

    Mat4 matrix() const {
        return Mat4::translate(position)
             * rotation.toMat4()
             * Mat4::scale(scale);
    }

    Vec3 forward() const { return rotation.rotate(Vec3{0, 0, 1}); }
    Vec3 up() const { return rotation.rotate(Vec3{0, 1, 0}); }
    Vec3 right() const { return rotation.rotate(Vec3{1, 0, 0}); }
};

}
```

### 4.2 Quaternion Math

**File: `include/math/quat.hpp`**
```cpp
#pragma once

#include "vec3.hpp"
#include "mat4.hpp"
#include <cmath>

namespace flightsim {

struct Quat {
    float w = 1, x = 0, y = 0, z = 0;

    static Quat identity() { return Quat{1, 0, 0, 0}; }

    static Quat fromAxisAngle(const Vec3& axis, float angle) {
        float half = angle * 0.5f;
        float s = std::sin(half);
        Vec3 n = axis.normalize();
        return Quat{std::cos(half), n.x * s, n.y * s, n.z * s};
    }

    Quat operator*(const Quat& q) const {
        return Quat{
            w*q.w - x*q.x - y*q.y - z*q.z,
            w*q.x + x*q.w + y*q.z - z*q.y,
            w*q.y - x*q.z + y*q.w + z*q.x,
            w*q.z + x*q.y - y*q.x + z*q.w
        };
    }

    Vec3 rotate(const Vec3& v) const {
        Vec3 u{x, y, z};
        return u * (2.0f * u.dot(v))
             + v * (w*w - u.dot(u))
             + u.cross(v) * (2.0f * w);
    }

    Quat normalized() const {
        float len = std::sqrt(w*w + x*x + y*y + z*z);
        return Quat{w/len, x/len, y/len, z/len};
    }

    Mat4 toMat4() const {
        Mat4 m;
        float xx = x*x, yy = y*y, zz = z*z;
        float xy = x*y, xz = x*z, yz = y*z;
        float wx = w*x, wy = w*y, wz = w*z;

        m.m[0] = 1 - 2*(yy + zz);
        m.m[1] = 2*(xy + wz);
        m.m[2] = 2*(xz - wy);

        m.m[4] = 2*(xy - wz);
        m.m[5] = 1 - 2*(xx + zz);
        m.m[6] = 2*(yz + wx);

        m.m[8] = 2*(xz + wy);
        m.m[9] = 2*(yz - wx);
        m.m[10] = 1 - 2*(xx + yy);

        return m;
    }
};

}
```

---

## Phase 5: Data-Driven Design

### 5.1 Asset Store

**File: `include/assets/asset_store.hpp`**
```cpp
#pragma once

#include "core/types.hpp"
#include "graphics/shader.hpp"
#include "graphics/mesh.hpp"
#include <unordered_map>
#include <memory>
#include <string>

namespace flightsim {

class AssetStore {
public:
    // Shaders
    bool loadShader(const std::string& name,
                    const std::string& vertPath,
                    const std::string& fragPath);
    Shader* getShader(const std::string& name);

    // Meshes
    bool loadMesh(const std::string& name, const std::vector<float>& vertices);
    Mesh* getMesh(const std::string& name);

    void unloadAll();

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
};

}
```

### 5.2 Config Files

**File: `assets/config/simulator.json`**
```json
{
    "window": {
        "width": 1920,
        "height": 1080,
        "title": "Flight Simulator",
        "vsync": true
    },
    "graphics": {
        "fov": 60,
        "farPlane": 5000
    }
}
```

**File: `assets/config/aircraft/cessna.json`**
```json
{
    "name": "Cessna 172",
    "speed": {
        "min": 25,
        "max": 70
    },
    "controlRates": {
        "pitch": 1.2,
        "yaw": 0.8,
        "roll": 1.5
    },
    "limits": {
        "maxPitch": 1.0,
        "minAltitude": 5
    },
    "throttleResponse": 0.5
}
```

**File: `assets/config/controls.json`**
```json
{
    "axes": {
        "pitch": {"positive": "W", "negative": "S"},
        "yaw": {"positive": "A", "negative": "D"},
        "roll": {"positive": "Q", "negative": "E"}
    },
    "buttons": {
        "throttle_up": "Space",
        "throttle_down": "LeftShift",
        "brake": "B",
        "quit": "Escape"
    }
}
```

**File: `assets/shaders/basic.vert`**
```glsl
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;
uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
```

**File: `assets/shaders/basic.frag`**
```glsl
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
```

---

## API Examples

### Basic Setup

```cpp
int main() {
    Simulator sim;
    sim.init({.windowWidth = 1920, .windowHeight = 1080});

    // Load assets
    sim.assets().loadShader("basic", "assets/shaders/basic.vert",
                                      "assets/shaders/basic.frag");

    // Generate terrain
    sim.terrain().generate({.gridSize = 100, .scale = 10.0f});

    // Spawn player
    sim.aircraft().spawnPlayer("assets/config/aircraft/cessna.json");

    // Run
    sim.run();
    sim.shutdown();
}
```

### Spawning Multiple Aircraft

```cpp
// Player
sim.aircraft().spawnPlayer("assets/config/aircraft/cessna.json");

// AI wingman
auto& wingman = sim.aircraft().spawn("assets/config/aircraft/cessna.json");
wingman.model.state().position = {100, 150, 50};
wingman.name = "Wingman1";

// More AI
for (int i = 0; i < 5; i++) {
    auto& ai = sim.aircraft().spawn("assets/config/aircraft/f16.json");
    ai.model.state().position = {i * 50.0f, 200, 500};
}
```

### Accessing Modules

```cpp
// In update loop or custom tick callback
void Simulator::tick(float dt) {
    // Check input
    if (input().isKeyPressed(GLFW_KEY_C)) {
        camera().setMode(CameraMode::Free);
    }

    // Query terrain height
    float groundHeight = terrain().heightAt(x, z);

    // Get player position
    if (auto* player = aircraft().player()) {
        Vec3 pos = player->model.position();
    }
}
```

### Camera Control

```cpp
// Setup
sim.camera().setMode(CameraMode::Chase);
sim.camera().setChaseDistance(30.0f);
sim.camera().setChaseHeight(12.0f);

// Switch modes
if (sim.input().isKeyPressed(GLFW_KEY_V)) {
    static int mode = 0;
    mode = (mode + 1) % 2;
    sim.camera().setMode(mode == 0 ? CameraMode::Chase : CameraMode::Free);
}
```

---

## File-by-File Changes

### Files to CREATE

| File | Purpose | Priority |
|------|---------|----------|
| `include/core/simulator.hpp` | Main class | P0 |
| `include/core/module.hpp` | Base module interface | P0 |
| `include/core/types.hpp` | Handles, IDs | P0 |
| `include/modules/input_module.hpp` | Input system | P0 |
| `include/modules/aircraft_module.hpp` | Aircraft management | P0 |
| `include/modules/terrain_module.hpp` | Terrain | P0 |
| `include/modules/camera_module.hpp` | Camera logic | P0 |
| `include/modules/render_module.hpp` | Core rendering | P0 |
| `include/simulation/flight_model.hpp` | Flight dynamics | P1 |
| `include/simulation/flight_params.hpp` | Aircraft config | P1 |
| `include/simulation/aircraft_state.hpp` | Aircraft data | P1 |
| `include/graphics/render_context.hpp` | View/proj matrices | P1 |
| `include/graphics/mesh_builder.hpp` | Procedural meshes | P1 |
| `include/world/transform.hpp` | Transform component | P1 |
| `include/math/quat.hpp` | Quaternions | P1 |
| `include/assets/asset_store.hpp` | Asset registry | P2 |
| `include/assets/shader_loader.hpp` | Load from files | P2 |
| `assets/shaders/basic.vert` | Vertex shader | P2 |
| `assets/shaders/basic.frag` | Fragment shader | P2 |
| `assets/config/aircraft/cessna.json` | Aircraft data | P3 |
| `assets/config/controls.json` | Key bindings | P3 |

### Files to MODIFY

| File | Changes |
|------|---------|
| `src/main.cpp` | Complete rewrite (minimal) |
| `include/graphics/shader.hpp` | Add `data()` to Mat4 |
| `include/math/mat4.hpp` | Add `data()` method |
| `CMakeLists.txt` | Add new sources |

### Files to REMOVE (after migration)

| File | Reason |
|------|--------|
| `include/game/aircraft.hpp` | Replaced by module + flight_model |
| `include/game/terrain.hpp` | Replaced by terrain_module |
| `src/game/aircraft.cpp` | Replaced |
| `src/game/terrain.cpp` | Replaced |

---

## Testing Strategy

### Unit Tests (No OpenGL)

```cpp
// test_flight_model.cpp
#include "simulation/flight_model.hpp"
#include <cassert>
#include <cmath>

void test_forward_movement() {
    FlightModel model;
    model.init(FlightParams{});

    FlightControls controls{};
    controls.throttle = 0.5f;

    Vec3 startPos = model.position();
    model.update(1.0f, controls);  // 1 second

    // Should have moved forward
    assert(model.position().z > startPos.z);
}

void test_pitch_affects_direction() {
    FlightModel model;
    model.init(FlightParams{});

    FlightControls controls{};
    controls.throttle = 0.5f;
    controls.pitch = 1.0f;  // Nose up

    float startAlt = model.altitude();
    for (int i = 0; i < 60; i++) {
        model.update(1.0f/60.0f, controls);
    }

    // Should have gained altitude
    assert(model.altitude() > startAlt);
}

int main() {
    test_forward_movement();
    test_pitch_affects_direction();
    return 0;
}
```

---

## Migration Path

### Core
1. Create `Module` base class
2. Create `Simulator` with basic tick loop
3. Create `InputModule` and `RenderModule`
4. New `main.cpp` that uses Simulator

### Simulation
1. Create `FlightModel`, `FlightParams`, `AircraftState`
2. Create `AircraftModule`
3. Port existing aircraft logic to new structure
4. Write flight model tests

### Rendering
1. Create `CameraModule` with chase camera
2. Create `TerrainModule`
3. Move shaders to files
4. Create `AssetStore`

### Polish
1. Load configs from JSON
2. Add more camera modes
3. Clean up old code
4. Documentation

---

## Module Tick Order Reference

```cpp
// UPDATE PHASE (modify state)
inputModule.update(dt);      // 1. Gather player input
aircraftModule.update(dt);   // 2. Update aircraft physics
terrainModule.update(dt);    // 3. Update terrain (LOD, streaming)
cameraModule.update(dt);     // 4. Follow aircraft

// RENDER PHASE (draw to screen)
renderModule.render();       // 1. Clear screen
terrainModule.render();      // 2. Draw terrain (back to front)
aircraftModule.render();     // 3. Draw aircraft
// uiModule.render();        // 4. Draw UI (future)
```

---

## Success Criteria

After refactor:

- [ ] Adding a new aircraft = one JSON file
- [ ] Adding a new module = implement Update()/Render()
- [ ] Flight model runs without OpenGL
- [ ] Camera follows player smoothly
- [ ] Can spawn multiple aircraft
- [ ] Each system is in one file, easy to understand
- [ ] Main loop is ~10 lines
