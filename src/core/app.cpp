#include "core/app.hpp"
#include "core/properties/property_paths.hpp"
#include "core/sim_subsystem.hpp"
#include "ui/anchor.hpp"
#include "graphics/glad.h"
#include "graphics/mesh_builder.hpp"
#include "aircraft/aircraft.hpp"
#include "utils/config_loader.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <chrono>
#include <iomanip>
#include <filesystem>

namespace nuage {

bool App::init(const AppConfig& config) {
    if (!initWindow(config)) return false;

    m_assets = std::make_shared<AssetStore>();
    m_subsystems.add(m_assets);

    m_input = std::make_shared<Input>();
    m_input->setWindow(m_window);
    m_subsystems.add(m_input);

    m_audio = std::make_shared<AudioSubsystem>();
    m_subsystems.add(m_audio);

    m_ui = std::make_shared<UIManager>();
    m_ui->setApp(this);
    m_subsystems.add(m_ui);

    m_subsystems.add(std::make_shared<SimSubsystem>());

    m_subsystems.initAll();

    FlightConfig flight;
    flight.aircraftPath = "assets/config/aircraft/c172p.json";
    flight.terrainPath = "assets/config/terrain_flightgear.json";
    if (!startFlight(flight)) {
        std::cerr << "Failed to start flight session" << std::endl;
        return false;
    }

    m_lastFrameTime = static_cast<float>(glfwGetTime());
    return true;
}

bool App::startFlight(const FlightConfig& config) {
    endFlight();

    m_session = std::make_unique<FlightSession>(this, config);
    if (!m_session->init()) {
        m_session.reset();
        return false;
    }

    m_ui->setAircraft(&m_session->aircraft());
    setPaused(false);
    return true;
}

void App::endFlight() {
    if (m_session) {
        m_session->shutdown();
        m_session.reset();
    }
    m_physicsAccumulator = 0.0f;
    if (m_ui) m_ui->setAircraft(nullptr);
    PropertyBus::global().set(Properties::Sim::DEBUG_VISIBLE, false);
}

void App::run() {
    using clock = std::chrono::high_resolution_clock;

    while (!m_shouldQuit && !glfwWindowShouldClose(m_window)) {
        auto frameStart = clock::now();

        float now = static_cast<float>(glfwGetTime());
        m_deltaTime = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_time = now;

        auto inputStart = clock::now();
        m_subsystems.updateAll(m_deltaTime);
        auto inputEnd = clock::now();

        if (PropertyBus::global().get(Properties::Sim::QUIT_REQUESTED, false)) {
            m_shouldQuit = true;
            continue;
        }

        if (m_session) {
            if (PropertyBus::global().get("sim/commands/toggle-camera", false)) {
                m_session->camera().toggleOrbitMode();
                PropertyBus::global().set("sim/commands/toggle-camera", false);
            }

            updatePhysics();

            if (m_session) {
                m_session->update(m_deltaTime);
                float alpha = m_physicsAccumulator / FIXED_DT;
                m_session->camera().update(m_deltaTime, m_session->aircraft().player(), alpha);
                m_session->camera().clampToGround(m_session->terrain(), 1.5f);
            }
        }

        auto renderStart = clock::now();
        float alpha = m_physicsAccumulator / FIXED_DT;
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_session) {
            m_session->render(alpha);
        }

        m_ui->render();
        auto renderEnd = clock::now();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
        auto frameEnd = clock::now();

        auto toMs = [](const clock::time_point& start, const clock::time_point& end) {
            return std::chrono::duration<double, std::milli>(end - start).count();
        };

        FrameProfile profile;
        profile.frameMs = static_cast<float>(toMs(frameStart, frameEnd));
        profile.inputMs = static_cast<float>(toMs(inputStart, inputEnd));
        profile.physicsMs = static_cast<float>(toMs(inputEnd, renderStart)); 
        profile.renderMs = static_cast<float>(toMs(renderStart, renderEnd));

        updateFrameStats(profile);
    }
}

void App::shutdown() {
    endFlight();
    m_subsystems.shutdownAll();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool App::initWindow(const AppConfig& config) {
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
    
    return true;
}

void App::setPaused(bool paused) {
    PropertyBus::global().set(Properties::Sim::PAUSED, paused);
    m_physicsAccumulator = 0.0f;
}

void App::updatePhysics() {
    bool paused = PropertyBus::global().get(Properties::Sim::PAUSED, false);
    if (paused || !m_session) {
        return;
    }
    m_physicsAccumulator += m_deltaTime;
    while (m_physicsAccumulator >= FIXED_DT) {
        m_session->aircraft().fixedUpdate(FIXED_DT);
        m_session->aircraft().applyGroundCollision(m_session->terrain());
        m_physicsAccumulator -= FIXED_DT;
    }
}

void App::updateFrameStats(const FrameProfile& profile) {
    m_framesSinceFps++;
    m_totalFrames++;
    m_fpsTimer += m_deltaTime;

    m_profileAccum.frames++;
    m_profileAccum.frameMs += profile.frameMs;
    m_profileAccum.inputMs += profile.inputMs;
    m_profileAccum.physicsMs += profile.physicsMs;
    m_profileAccum.renderMs += profile.renderMs;

    if (m_fpsTimer < 1.0f) return;

    double invFrames = (m_profileAccum.frames > 0) ? (1.0 / m_profileAccum.frames) : 0.0;
    m_lastFps = static_cast<float>(m_framesSinceFps) / m_fpsTimer;
    m_lastProfile.frameMs = static_cast<float>(m_profileAccum.frameMs * invFrames);
    m_lastProfile.inputMs = static_cast<float>(m_profileAccum.inputMs * invFrames);
    m_lastProfile.physicsMs = static_cast<float>(m_profileAccum.physicsMs * invFrames);
    m_lastProfile.renderMs = static_cast<float>(m_profileAccum.renderMs * invFrames);

    m_framesSinceFps = 0;
    m_fpsTimer = 0.0f;
    m_profileAccum = {};
}

} // namespace nuage
