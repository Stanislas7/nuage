#pragma once

#include "input/input.hpp"
#include "aircraft/aircraft.hpp"
#include "graphics/asset_store.hpp"
#include "environment/atmosphere.hpp"
#include "graphics/camera.hpp"
#include "scenery/scenery.hpp"
#include "ui/ui_manager.hpp"
#include <cstdint>

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
    struct FrameProfile {
        float frameMs = 0.0f;
        float inputMs = 0.0f;
        float physicsMs = 0.0f;
        float atmosphereMs = 0.0f;
        float cameraMs = 0.0f;
        float hudMs = 0.0f;
        float renderMs = 0.0f;
        float swapMs = 0.0f;
    };

    struct FrameProfileAccum {
        double frameMs = 0.0;
        double inputMs = 0.0;
        double physicsMs = 0.0;
        double atmosphereMs = 0.0;
        double cameraMs = 0.0;
        double hudMs = 0.0;
        double renderMs = 0.0;
        double swapMs = 0.0;
        int frames = 0;
    };

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
    Shader* m_skyShader = nullptr;
    unsigned int m_skyVao = 0;

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
    void render(float alpha);
    void printDebugInfo();
    void updateFrameStats(const FrameProfile& profile);
    
    float m_lastDebugTime = 0.0f;

    float m_lastFps = 0.0f;
    float m_fpsTimer = 0.0f;
    int m_framesSinceFps = 0;
    std::uint64_t m_totalFrames = 0;
    FrameProfile m_lastProfile{};
    FrameProfileAccum m_profileAccum{};
};

}
