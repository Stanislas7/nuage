# Nuage Flight Simulator - Refactoring Plan

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                           App                                │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐           │
│  │  Input  │ │  Asset  │ │ Camera  │ │  World  │           │
│  │ Manager │ │ Manager │ │ Manager │ │ Manager │           │
│  └────┬────┘ └─────────┘ └────┬────┘ └─────────┘           │
│       │                       │                              │
│       │ FlightInput           │ follows                      │
│       ▼                       ▼                              │
│  ┌─────────────────────────────────────────┐                │
│  │            AircraftManager              │                │
│  │  ┌─────────────────────────────────┐   │                │
│  │  │           Aircraft              │   │                │
│  │  │  ┌───────────────────────────┐ │   │                │
│  │  │  │       PropertyBus         │ │   │                │
│  │  │  │  input/pitch, velocity/*, │ │   │                │
│  │  │  │  engine/thrust, fuel/qty  │ │   │                │
│  │  │  └─────────▲───────▲─────────┘ │   │                │
│  │  │       read │       │ write     │   │                │
│  │  │  ┌─────────┴───┐ ┌─┴─────────┐ │   │                │
│  │  │  │   Flight    │ │  Engine   │ │   │                │
│  │  │  │  Dynamics   │ │  System   │ │   │                │
│  │  │  └─────────────┘ └───────────┘ │   │                │
│  │  └─────────────────────────────────┘   │                │
│  └─────────────────────────────────────────┘                │
│       │                                                      │
│       │ queries density                                      │
│       ▼                                                      │
│  ┌─────────────┐                                            │
│  │ Atmosphere  │                                            │
│  │  Manager    │                                            │
│  └─────────────┘                                            │
└─────────────────────────────────────────────────────────────┘
```

## New Folder Structure

Flat structure with `.hpp` and `.cpp` together — easy to navigate, no mental overhead.

```
nuage/
├── app/
│   ├── App.hpp / .cpp              # The Grand Manager
│   └── main.cpp
├── managers/
│   ├── InputManager.hpp / .cpp
│   ├── AssetManager.hpp / .cpp
│   ├── WorldManager.hpp / .cpp
│   ├── AircraftManager.hpp / .cpp
│   ├── AtmosphereManager.hpp / .cpp
│   └── CameraManager.hpp / .cpp
├── aircraft/
│   ├── Aircraft.hpp / .cpp          # The container
│   ├── IAircraftSystem.hpp
│   ├── PropertyBus.hpp / .cpp       # Inter-system data hub
│   └── systems/
│       ├── FlightDynamics.hpp / .cpp
│       ├── EngineSystem.hpp / .cpp
│       └── FuelSystem.hpp / .cpp
├── graphics/
│   ├── Mesh.hpp / .cpp
│   ├── Shader.hpp / .cpp
│   ├── MeshBuilder.hpp / .cpp
│   └── Renderer.hpp / .cpp
├── math/
│   ├── Vec3.hpp / .cpp
│   ├── Mat4.hpp / .cpp
│   └── Quat.hpp / .cpp
├── world/
│   ├── Terrain.hpp / .cpp
│   ├── TerrainChunk.hpp / .cpp
│   └── Airport.hpp / .cpp
└── assets/
    ├── shaders/
    ├── meshes/
    └── configs/
```

---

## The Managers

### 1. App (The Grand Manager)

```cpp
class App {
public:
    bool init();
    void run();          // The main loop
    void shutdown();

    // Manager access
    InputManager&      input()      { return m_input; }
    AssetManager&      assets()     { return m_assets; }
    WorldManager&      world()      { return m_world; }
    AircraftManager&   aircraft()   { return m_aircraft; }
    AtmosphereManager& atmosphere() { return m_atmosphere; }
    CameraManager&     camera()     { return m_camera; }

    // Time
    float time() const { return m_time; }
    float dt() const { return m_deltaTime; }

private:
    GLFWwindow* m_window = nullptr;

    // Managers (order = init/update order)
    InputManager      m_input;
    AssetManager      m_assets;
    WorldManager      m_world;
    AircraftManager   m_aircraft;
    AtmosphereManager m_atmosphere;
    CameraManager     m_camera;

    float m_time = 0;
    float m_deltaTime = 0;

    // Fixed timestep for physics
    float m_physicsAccumulator = 0;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
};
```

**Main loop:**
```cpp
void App::run() {
    while (!glfwWindowShouldClose(m_window)) {
        float now = glfwGetTime();
        m_deltaTime = now - m_time;
        m_time = now;

        // Input at frame rate
        m_input.update();

        // Physics at fixed rate
        m_physicsAccumulator += m_deltaTime;
        while (m_physicsAccumulator >= FIXED_DT) {
            m_aircraft.fixedUpdate(FIXED_DT);
            m_physicsAccumulator -= FIXED_DT;
        }

        // Everything else
        m_world.update(m_deltaTime);
        m_atmosphere.update(m_deltaTime);
        m_camera.update(m_deltaTime);

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_world.render();
        m_aircraft.render();
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}
```

---

### 2. InputManager

Translates raw input → abstract actions. No `glfwGetKey` in aircraft code.

```cpp
struct FlightInput {
    float pitch = 0;      // -1 to 1
    float roll = 0;
    float yaw = 0;
    float throttle = 0;   // 0 to 1
    bool toggleGear = false;
    bool toggleFlaps = false;
};

class InputManager {
public:
    void init(GLFWwindow* window);
    void update();

    const FlightInput& flight() const { return m_flight; }

    // For later: swap input source
    void setSource(InputSource source);  // Keyboard, Joystick, Network

private:
    GLFWwindow* m_window = nullptr;
    FlightInput m_flight;
    float m_throttleAccum = 0.3f;  // Throttle is accumulated
};
```

---

### 3. AssetManager

Load once, use everywhere.

```cpp
class AssetManager {
public:
    void init();
    void shutdown();

    Shader* loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    Mesh*   loadMesh(const std::string& name, const std::string& objPath);
    
    Shader* getShader(const std::string& name);
    Mesh*   getMesh(const std::string& name);

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
};
```

---

### 4. WorldManager

Handles terrain chunks and the floating-point origin problem.

```cpp
class WorldManager {
public:
    void init(App* app);
    void update(float dt);
    void render();

    // Origin shifting for large worlds
    Vec3 worldOrigin() const { return m_origin; }
    void checkOriginShift(const Vec3& playerPos);

    // Terrain queries
    float getGroundHeight(float x, float z) const;

private:
    App* m_app = nullptr;
    Vec3 m_origin{0, 0, 0};
    std::vector<TerrainChunk> m_chunks;

    static constexpr float ORIGIN_SHIFT_THRESHOLD = 10000.0f;
};
```

---

### 5. AircraftManager

Spawns and manages all aircraft.

```cpp
class AircraftManager {
public:
    void init(App* app);
    void fixedUpdate(float dt);  // Physics
    void render();
    void shutdown();

    // Spawning
    Aircraft* spawnPlayer(const std::string& configPath);
    Aircraft* spawnAI(const std::string& configPath, const Vec3& position);

    // Access
    Aircraft* player() { return m_player; }
    const std::vector<std::unique_ptr<Aircraft>>& all() const { return m_aircraft; }

    // Cleanup
    void destroy(Aircraft* aircraft);

private:
    App* m_app = nullptr;
    std::vector<std::unique_ptr<Aircraft>> m_aircraft;
    Aircraft* m_player = nullptr;
};
```

---

### 6. AtmosphereManager

Weather, air density, lighting.

```cpp
class AtmosphereManager {
public:
    void init();
    void update(float dt);

    // Queries for physics
    float getAirDensity(float altitude) const;
    Vec3  getWind(const Vec3& position) const;

    // Queries for rendering
    float getTimeOfDay() const { return m_timeOfDay; }
    Vec3  getSunDirection() const;

    // Settings
    void setTimeOfDay(float hours);  // 0-24
    void setWind(float speed, float heading);

private:
    float m_timeOfDay = 12.0f;  // Noon
    float m_windSpeed = 0;
    float m_windHeading = 0;
};
```

---

### 7. CameraManager

Multiple views, smooth transitions.

```cpp
enum class CameraMode { Cockpit, Chase, Tower, Free };

class CameraManager {
public:
    void init(App* app);
    void update(float dt);

    void setMode(CameraMode mode);
    void setTarget(Aircraft* target);

    Mat4 viewMatrix() const { return m_view; }
    Mat4 projectionMatrix() const { return m_projection; }
    Mat4 viewProjection() const { return m_projection * m_view; }

private:
    App* m_app = nullptr;
    Aircraft* m_target = nullptr;
    CameraMode m_mode = CameraMode::Chase;

    Vec3 m_position;
    Vec3 m_lookAt;
    Mat4 m_view;
    Mat4 m_projection;

    // Smooth follow
    float m_followDistance = 15.0f;
    float m_followHeight = 5.0f;
    float m_smoothing = 5.0f;
};
```

---

## Aircraft Architecture

### Aircraft (The Container)

```cpp
class Aircraft {
public:
    void init(const std::string& configPath, App* app);
    void update(float dt, const FlightInput& input);
    void render(const Mat4& viewProjection);

    // State access via PropertyBus
    PropertyBus& state() { return m_state; }
    
    // Convenience
    Vec3 position() const;
    Quat orientation() const;
    float airspeed() const;

private:
    PropertyBus m_state;
    std::vector<std::unique_ptr<IAircraftSystem>> m_systems;
    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
};
```

### IAircraftSystem

```cpp
class IAircraftSystem {
public:
    virtual ~IAircraftSystem() = default;
    virtual const char* name() const = 0;
    virtual void init(PropertyBus* state, App* app) = 0;
    virtual void update(float dt) = 0;
};
```

### PropertyBus (The Data Hub)

Systems read/write here instead of talking to each other directly.

```cpp
class PropertyBus {
public:
    void   set(const std::string& key, double value);
    double get(const std::string& key, double fallback = 0) const;
    bool   has(const std::string& key) const;

private:
    std::unordered_map<std::string, double> m_data;
};
```

**Standard properties:**
| Path | Description |
|------|-------------|
| `input/pitch` | Stick input (-1 to 1) |
| `input/throttle` | Throttle lever (0 to 1) |
| `position/x`, `y`, `z` | World position (m) |
| `velocity/airspeed` | Forward speed (m/s) |
| `engine/thrust` | Current thrust (N) |
| `engine/n1` | Turbine % |
| `fuel/quantity` | Fuel remaining (kg) |
| `atmosphere/density` | Air density at current alt |

---

## Migration Steps

| # | Task | Files Changed |
|---|------|---------------|
| 1 | Create `managers/` folder, move Module → Manager | Rename existing |
| 2 | Flatten include/src → single folders | All |
| 3 | Implement `PropertyBus` | New file |
| 4 | Refactor `Simulator` → `App` | Core loop |
| 5 | Add `AtmosphereManager` | New file |
| 6 | Extract `FlightDynamics` system | From FlightModel |
| 7 | Wire Aircraft to use systems | aircraft/ |
| 8 | Add fixed timestep to App | app/App.cpp |
| 9 | Delete old include/ structure | Cleanup |

---

## Why This Works

- **Writing logic is simple**: `AircraftManager::spawnPlayer()`, `AtmosphereManager::getAirDensity(alt)`
- **No spaghetti**: Systems don't know about each other, they just read/write to PropertyBus
- **Easy to extend**: Add `AudioManager`, `NetworkManager` later without touching existing code
- **Testable**: `FlightDynamics` can run without OpenGL, just feed it a PropertyBus
