#pragma once

#include "module.hpp"
#include "managers/input_module.hpp"
#include "managers/aircraft_module.hpp"
#include "managers/terrain_module.hpp"
#include "managers/camera_module.hpp"
#include "managers/render_module.hpp"
#include "managers/asset_store.hpp"
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
