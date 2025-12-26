#include "core/app.hpp"
#include "ui/anchor.hpp"
#include "graphics/glad.h"
#include "graphics/mesh_builder.hpp"
#include "aircraft/aircraft.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <string>
#include <chrono>
#include <iomanip>

namespace nuage {

bool App::init(const AppConfig& config) {
    if (!initWindow(config)) return false;

    m_input.init(m_window);
    m_assets.loadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
    m_assets.loadShader("textured", "assets/shaders/textured.vert", "assets/shaders/textured.frag");
    
    // Create terrain mesh
    auto terrainData = MeshBuilder::terrain(20000.0f, 40);
    m_assets.loadMesh("terrain", terrainData);

    m_atmosphere.init();
    m_aircraft.init(m_assets, m_atmosphere);
    m_camera.init(m_input);
    m_scenery.init(m_assets);
    
    m_camera.setAspect(static_cast<float>(config.windowWidth) / config.windowHeight);

    if (!m_ui.init(this)) {
        std::cerr << "Failed to initialize UI system" << std::endl;
        return false;
    }

    setupScene();
    setupHUD();

    m_lastFrameTime = static_cast<float>(glfwGetTime());
    return true;
}

void App::run() {
    using clock = std::chrono::high_resolution_clock;

    while (!m_shouldQuit && !glfwWindowShouldClose(m_window)) {
        auto frameStart = clock::now();

        float now = static_cast<float>(glfwGetTime());
        m_deltaTime = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_time = now;

        printDebugInfo();

        auto inputStart = clock::now();
        m_input.update(m_deltaTime);
        auto inputEnd = clock::now();

        if (m_input.isKeyPressed(GLFW_KEY_TAB)) {
            m_camera.toggleOrbitMode();
        }

        if (m_input.quitRequested()) {
            m_shouldQuit = true;
            continue;
        }

        auto physicsStart = clock::now();
        updatePhysics();
        auto physicsEnd = clock::now();
        
        float alpha = m_physicsAccumulator / FIXED_DT;

        auto atmosphereStart = clock::now();
        m_atmosphere.update(m_deltaTime);
        auto atmosphereEnd = clock::now();
        auto cameraStart = clock::now();
        m_camera.update(m_deltaTime, m_aircraft.player(), alpha);
        auto cameraEnd = clock::now();

        auto hudStart = clock::now();
        updateHUD();
        auto hudEnd = clock::now();
        auto renderStart = clock::now();
        render(alpha);
        auto renderEnd = clock::now();

        auto swapStart = clock::now();
        glfwSwapBuffers(m_window);
        glfwPollEvents();
        auto swapEnd = clock::now();

        auto toMs = [](const clock::time_point& start, const clock::time_point& end) {
            return std::chrono::duration<double, std::milli>(end - start).count();
        };

        FrameProfile profile;
        profile.frameMs = static_cast<float>(toMs(frameStart, swapEnd));
        profile.inputMs = static_cast<float>(toMs(inputStart, inputEnd));
        profile.physicsMs = static_cast<float>(toMs(physicsStart, physicsEnd));
        profile.atmosphereMs = static_cast<float>(toMs(atmosphereStart, atmosphereEnd));
        profile.cameraMs = static_cast<float>(toMs(cameraStart, cameraEnd));
        profile.hudMs = static_cast<float>(toMs(hudStart, hudEnd));
        profile.renderMs = static_cast<float>(toMs(renderStart, renderEnd));
        profile.swapMs = static_cast<float>(toMs(swapStart, swapEnd));

        updateFrameStats(profile);
    }
}

void App::shutdown() {
    m_aircraft.shutdown();
    m_ui.shutdown();
    m_assets.unloadAll();
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
    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
    
    return true;
}

void App::setupScene() {
    m_terrainMesh = m_assets.getMesh("terrain");
    m_terrainShader = m_assets.getShader("basic");
    
    m_scenery.loadConfig("assets/config/scenery.json");
}

void App::setupHUD() {
    auto setupText = [](Text& text, float x, float y, float scale, Anchor anchor, float padding) {
        text.scaleVal(scale);
        text.pos(x, y);
        text.colorR(1, 1, 1);
        text.anchorMode(anchor);
        text.paddingValue(padding);
    };

    m_altitudeText = &m_ui.text("ALT: 0.0 m");
    setupText(*m_altitudeText, 20, 20, 2.0f, Anchor::TopLeft, 0.0f);

    m_airspeedText = &m_ui.text("SPD: 0 kts");
    setupText(*m_airspeedText, 20, 70, 2.0f, Anchor::TopLeft, 0.0f);

    m_headingText = &m_ui.text("HDG: 000");
    setupText(*m_headingText, 20, 120, 2.0f, Anchor::TopLeft, 0.0f);

    m_positionText = &m_ui.text("POS: 0, 0, 0");
    setupText(*m_positionText, 20, 170, 2.0f, Anchor::TopLeft, 0.0f);

}

void App::updatePhysics() {
    m_physicsAccumulator += m_deltaTime;
    while (m_physicsAccumulator >= FIXED_DT) {
        m_aircraft.fixedUpdate(FIXED_DT, m_input);
        m_physicsAccumulator -= FIXED_DT;
    }
}

void App::updateHUD() {
    Aircraft::Instance* player = m_aircraft.player();
    if (player && m_altitudeText && m_airspeedText && m_headingText && m_positionText) {
        Vec3 pos = player->position();
        float airspeed = player->airspeed();
        Vec3 fwd = player->forward();

        float heading = std::atan2(fwd.x, fwd.z) * 180.0f / 3.14159265f;
        if (heading < 0) heading += 360.0f;

        float airspeedKnots = airspeed * 1.94384f;

        auto fmt = [](float val) {
            std::string s = std::to_string(val);
            return s.substr(0, s.find('.') + 2);
        };

        m_altitudeText->content("ALT: " + fmt(pos.y) + " m");
        m_airspeedText->content("SPD: " + fmt(airspeedKnots) + " kts");
        m_headingText->content("HDG: " + std::to_string(static_cast<int>(heading)));
        m_positionText->content("POS: " + std::to_string(static_cast<int>(pos.x)) + ", " +
                                std::to_string(static_cast<int>(pos.y)) + ", " +
                                std::to_string(static_cast<int>(pos.z)));
    }

    m_ui.update();
}

void App::render(float alpha) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 vp = m_camera.viewProjection();

    if (m_terrainMesh && m_terrainShader) {
        m_terrainShader->use();
        m_terrainShader->setMat4("uMVP", vp);
        m_terrainMesh->draw();
    }
    
    m_scenery.render(vp);
    
    m_aircraft.render(vp, alpha);
    
    m_ui.draw();
}

void App::printDebugInfo() {
    if (m_time - m_lastDebugTime < 2.0f) return;
    m_lastDebugTime = m_time;

    std::cout << "\n=== DEBUG (t=" << m_time << ") ===\n";

    Aircraft::Instance* player = m_aircraft.player();
    if (!player) {
        std::cout << "[AIRCRAFT] Player is NULL!\n";
    } else {
        Vec3 pos = player->position();
        Vec3 fwd = player->forward();
        std::cout << "[AIRCRAFT] Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
        std::cout << "[AIRCRAFT] Forward:  (" << fwd.x << ", " << fwd.y << ", " << fwd.z << ")\n";
        std::cout << "[AIRCRAFT] Airspeed: " << player->airspeed() << " m/s\n";
    }

    Vec3 camPos = m_camera.position();
    std::cout << "[CAMERA] Position: (" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")\n";
    std::cout << "==========================\n" << std::flush;
}

void App::updateFrameStats(const FrameProfile& profile) {
    m_framesSinceFps++;
    m_totalFrames++;
    m_fpsTimer += m_deltaTime;

    m_profileAccum.frames++;
    m_profileAccum.frameMs += profile.frameMs;
    m_profileAccum.inputMs += profile.inputMs;
    m_profileAccum.physicsMs += profile.physicsMs;
    m_profileAccum.atmosphereMs += profile.atmosphereMs;
    m_profileAccum.cameraMs += profile.cameraMs;
    m_profileAccum.hudMs += profile.hudMs;
    m_profileAccum.renderMs += profile.renderMs;
    m_profileAccum.swapMs += profile.swapMs;

    if (m_fpsTimer < 1.0f) return;

    double invFrames = (m_profileAccum.frames > 0) ? (1.0 / m_profileAccum.frames) : 0.0;
    m_lastFps = static_cast<float>(m_framesSinceFps) / m_fpsTimer;
    m_lastProfile.frameMs = static_cast<float>(m_profileAccum.frameMs * invFrames);
    m_lastProfile.inputMs = static_cast<float>(m_profileAccum.inputMs * invFrames);
    m_lastProfile.physicsMs = static_cast<float>(m_profileAccum.physicsMs * invFrames);
    m_lastProfile.atmosphereMs = static_cast<float>(m_profileAccum.atmosphereMs * invFrames);
    m_lastProfile.cameraMs = static_cast<float>(m_profileAccum.cameraMs * invFrames);
    m_lastProfile.hudMs = static_cast<float>(m_profileAccum.hudMs * invFrames);
    m_lastProfile.renderMs = static_cast<float>(m_profileAccum.renderMs * invFrames);
    m_lastProfile.swapMs = static_cast<float>(m_profileAccum.swapMs * invFrames);

    std::cout << std::fixed << std::setprecision(1)
              << "FPS avg 1s: " << m_lastFps
              << " | Frame " << m_totalFrames
              << " | ms (frame " << m_lastProfile.frameMs
              << ", input " << m_lastProfile.inputMs
              << ", physics " << m_lastProfile.physicsMs
              << ", atmos " << m_lastProfile.atmosphereMs
              << ", camera " << m_lastProfile.cameraMs
              << ", hud " << m_lastProfile.hudMs
              << ", render " << m_lastProfile.renderMs
              << ", swap " << m_lastProfile.swapMs
              << ")\n";

    m_framesSinceFps = 0;
    m_fpsTimer = 0.0f;
    m_profileAccum = {};
}

}
