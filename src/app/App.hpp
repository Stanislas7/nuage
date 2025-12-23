#pragma once

#include "managers/input/InputManager.hpp"
#include "managers/aircraft/AircraftManager.hpp"
#include "managers/assets/asset_store.hpp"
#include "managers/atmosphere/AtmosphereManager.hpp"
#include "managers/camera/CameraManager.hpp"

struct GLFWwindow;

namespace nuage {

struct AppConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    const char* title = "Nuage";
    bool vsync = true;
};

class App {
public:
    bool init(const AppConfig& config = {});
    void run();
    void shutdown();

    InputManager& input() { return m_input; }
    AircraftManager& aircraft() { return m_aircraft; }
    AssetStore& assets() { return m_assets; }
    AtmosphereManager& atmosphere() { return m_atmosphere; }
    CameraManager& camera() { return m_camera; }

    float time() const { return m_time; }
    float dt() const { return m_deltaTime; }
    GLFWwindow* window() const { return m_window; }
    
    void quit() { m_shouldQuit = true; }

private:
    GLFWwindow* m_window = nullptr;

    InputManager m_input;
    AircraftManager m_aircraft;
    AssetStore m_assets;
    AtmosphereManager m_atmosphere;
    CameraManager m_camera;

    float m_time = 0.0f;
    float m_deltaTime = 0.0f;
    float m_lastFrameTime = 0.0f;
    bool m_shouldQuit = false;

    float m_physicsAccumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;

    Mesh* m_terrainMesh = nullptr;
    Shader* m_terrainShader = nullptr;
};

}
