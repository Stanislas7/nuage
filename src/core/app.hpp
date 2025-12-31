#pragma once

#include "input/input.hpp"
#include "graphics/asset_store.hpp"
#include "ui/ui_manager.hpp"
#include "ui/overlays/pause_overlay.hpp"
#include "ui/overlays/debug_overlay.hpp"
#include "audio/audio_subsystem.hpp"
#include "core/session/flight_config.hpp"
#include "core/session/flight_session.hpp"
#include "core/subsystem_manager.hpp"
#include <cstdint>
#include <memory>

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

    // Session Management
    bool startFlight(const FlightConfig& config);
    void endFlight();
    bool isFlightActive() const { return m_session != nullptr; }

    // Persistent Systems
    Input& input() { return *m_input; }
    UIManager& ui() { return *m_ui; }
    SubsystemManager& subsystems() { return m_subsystems; }
    const SubsystemManager& subsystems() const { return m_subsystems; }

    // Current Session Delegation
    FlightSession* session() { return m_session.get(); }

    float time() const { return m_time; }
    float dt() const { return m_deltaTime; }
    GLFWwindow* window() const { return m_window; }

    void setPaused(bool paused);
    
    void quit() { m_shouldQuit = true; }

private:
    struct FrameProfile {
        float frameMs = 0.0f;
        float inputMs = 0.0f;
        float physicsMs = 0.0f;
        float renderMs = 0.0f;
        float swapMs = 0.0f;
    };

    struct FrameProfileAccum {
        double frameMs = 0.0;
        double inputMs = 0.0;
        double physicsMs = 0.0;
        double renderMs = 0.0;
        double swapMs = 0.0;
        int frames = 0;
    };

    GLFWwindow* m_window = nullptr;

    // Persistent Engine Systems
    SubsystemManager m_subsystems;
    std::shared_ptr<Input> m_input;
    std::shared_ptr<UIManager> m_ui;
    std::shared_ptr<AssetStore> m_assets;
    std::shared_ptr<AudioSubsystem> m_audio;
    // Active Flight Session
    std::unique_ptr<FlightSession> m_session;

    float m_time = 0.0f;
    float m_deltaTime = 0.0f;
    float m_lastFrameTime = 0.0f;
    bool m_shouldQuit = false;

    float m_physicsAccumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;

    // Helpers
    bool initWindow(const AppConfig& config);
    void updatePhysics();
    void updateFrameStats(const FrameProfile& profile);
    
    float m_lastFps = 0.0f;
    float m_fpsTimer = 0.0f;
    int m_framesSinceFps = 0;
    std::uint64_t m_totalFrames = 0;
    FrameProfile m_lastProfile{};
    FrameProfileAccum m_profileAccum{};
};

}
