#pragma once

#include "input/input.hpp"
#include "aircraft/aircraft.hpp"
#include "graphics/asset_store.hpp"
#include "environment/atmosphere.hpp"
#include "graphics/camera.hpp"
#include "scenery/scenery.hpp"
#include "ui/ui_manager.hpp"

struct GLFWwindow;

namespace nuage {

class Text;

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

    Input& input() { return m_input; }
    Aircraft& aircraft() { return m_aircraft; }
    AssetStore& assets() { return m_assets; }
    Atmosphere& atmosphere() { return m_atmosphere; }
    Camera& camera() { return m_camera; }
    Scenery& scenery() { return m_scenery; }
    UIManager& ui() { return m_ui; }

    float time() const { return m_time; }
    float dt() const { return m_deltaTime; }
    GLFWwindow* window() const { return m_window; }
    
    void quit() { m_shouldQuit = true; }

private:
    GLFWwindow* m_window = nullptr;

    Input m_input;
    Aircraft m_aircraft;
    AssetStore m_assets;
    Atmosphere m_atmosphere;
    Camera m_camera;
    Scenery m_scenery;
    UIManager m_ui;

    float m_time = 0.0f;
    float m_deltaTime = 0.0f;
    float m_lastFrameTime = 0.0f;
    bool m_shouldQuit = false;

    float m_physicsAccumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;

    Mesh* m_terrainMesh = nullptr;
    Shader* m_terrainShader = nullptr;

    Text* m_altitudeText = nullptr;
    Text* m_airspeedText = nullptr;
    Text* m_headingText = nullptr;
    Text* m_positionText = nullptr;
    
    // Helpers
    bool initWindow(const AppConfig& config);
    void setupScene();
    void setupHUD();
    
    void updatePhysics();
    void updateHUD();
    void render();
    void printDebugInfo();
    
    float m_lastDebugTime = 0.0f;
};

}
