#include "app/App.hpp"
#include "ui/Anchor.hpp"
#include "graphics/glad.h"
#include "graphics/mesh_builder.hpp"
#include "aircraft/Aircraft.hpp"

#define GLFW_INCLUDE_NONE // Tell GLFW not to include its own OpenGL headers
#include <GLFW/glfw3.h>
#include <iostream>

namespace nuage {

static void debugPrint(App& app, float& lastDebugTime) {
    float now = app.time();
    if (now - lastDebugTime < 2.0f) return;
    lastDebugTime = now;

    std::cout << "\n=== DEBUG (t=" << now << ") ===\n";

    Aircraft* player = app.aircraft().player();
    if (!player) {
        std::cout << "[AIRCRAFT] Player is NULL!\n";
    } else {
        Vec3 pos = player->position();
        Vec3 fwd = player->forward();
        std::cout << "[AIRCRAFT] Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
        std::cout << "[AIRCRAFT] Forward:  (" << fwd.x << ", " << fwd.y << ", " << fwd.z << ")\n";
        std::cout << "[AIRCRAFT] Airspeed: " << player->airspeed() << " m/s\n";
        std::cout << "[AIRCRAFT] Has mesh: " << (player->mesh() ? "YES" : "NO") << "\n";
        std::cout << "[AIRCRAFT] Has shader: " << (player->shader() ? "YES" : "NO") << "\n";
    }

    Vec3 camPos = app.camera().position();
    std::cout << "[CAMERA] Position: (" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")\n";
    std::cout << "==========================\n" << std::flush;
}

bool App::init(const AppConfig& config) {
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

    m_input.init(m_window);
    m_aircraft.init(this);
    m_atmosphere.init();
    m_camera.init(this);

    if (!m_ui.init(this)) {
        std::cerr << "Failed to initialize UI system" << std::endl;
        return false;
    }

    m_camera.setAspect(static_cast<float>(config.windowWidth) / config.windowHeight);

    AircraftMeshSpecs specs;
    m_assets.loadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
    m_assets.loadMesh("aircraft", MeshBuilder::aircraft(specs));

    auto terrainData = MeshBuilder::terrain(20000.0f, 40);
    m_assets.loadMesh("terrain", terrainData);
    m_terrainMesh = m_assets.getMesh("terrain");
    m_terrainShader = m_assets.getShader("basic");

    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);

    m_altitudeText = &m_ui.text("ALT: 0.0 m");
    m_altitudeText->scaleVal(2.0f);
    m_altitudeText->pos(20, 20);
    m_altitudeText->colorR(1, 1, 1);

    m_airspeedText = &m_ui.text("SPD: 0.0 m/s");
    m_airspeedText->scaleVal(2.0f);
    m_airspeedText->pos(20, 70);
    m_airspeedText->colorR(1, 1, 1);

    m_headingText = &m_ui.text("HDG: 000");
    m_headingText->scaleVal(2.0f);
    m_headingText->pos(20, 120);
    m_headingText->colorR(1, 1, 1);

    m_positionText = &m_ui.text("POS: 0, 0, 0");
    m_positionText->scaleVal(2.0f);
    m_positionText->pos(20, 170);
    m_positionText->colorR(1, 1, 1);

    m_lastFrameTime = static_cast<float>(glfwGetTime());
    return true;
}

void App::run() {
    float lastDebugTime = 0.0f;
    while (!m_shouldQuit && !glfwWindowShouldClose(m_window)) {
        float now = static_cast<float>(glfwGetTime());
        m_deltaTime = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_time = now;

        debugPrint(*this, lastDebugTime);

        m_input.update(m_deltaTime);

        if (m_input.isKeyPressed(GLFW_KEY_TAB)) {
            m_camera.toggleOrbitMode();
        }

        if (m_input.quitRequested()) {
            m_shouldQuit = true;
            continue;
        }

        m_physicsAccumulator += m_deltaTime;
        while (m_physicsAccumulator >= FIXED_DT) {
            m_aircraft.fixedUpdate(FIXED_DT);
            m_physicsAccumulator -= FIXED_DT;
        }

        m_atmosphere.update(m_deltaTime);
        m_camera.update(m_deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_terrainMesh && m_terrainShader) {
            m_terrainShader->use();
            m_terrainShader->setMat4("uMVP", m_camera.viewProjection());
            m_terrainMesh->draw();
        }

        m_aircraft.render();

        Aircraft* player = m_aircraft.player();
        if (player && m_altitudeText && m_airspeedText && m_headingText && m_positionText) {
            Vec3 pos = player->position();
            float airspeed = player->airspeed();
            Vec3 fwd = player->forward();

            float heading = std::atan2(fwd.x, fwd.z) * 180.0f / 3.14159265f;
            if (heading < 0) heading += 360.0f;

            m_altitudeText->content("ALT: " + std::to_string(pos.y).substr(0, std::to_string(pos.y).find('.') + 2) + " m");
            m_airspeedText->content("SPD: " + std::to_string(airspeed).substr(0, std::to_string(airspeed).find('.') + 2) + " m/s");
            m_headingText->content("HDG: " + std::to_string(static_cast<int>(heading)));
            m_positionText->content("POS: " + std::to_string(static_cast<int>(pos.x)) + ", " + std::to_string(static_cast<int>(pos.y)) + ", " + std::to_string(static_cast<int>(pos.z)));
        }

        m_ui.update();
        m_ui.draw();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void App::shutdown() {
    m_aircraft.shutdown();
    m_ui.shutdown();
    m_assets.unloadAll();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}
