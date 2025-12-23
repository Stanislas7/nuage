#include "app/App.hpp"
#include "graphics/glad.h"
#include "graphics/mesh_builder.hpp"
#include "aircraft/Aircraft.hpp"
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

    m_camera.setAspect(static_cast<float>(config.windowWidth) / config.windowHeight);

    AircraftMeshSpecs specs;
    m_assets.loadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
    m_assets.loadMesh("aircraft", MeshBuilder::aircraft(specs));

    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);

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
        m_aircraft.render();
        
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void App::shutdown() {
    m_aircraft.shutdown();
    m_assets.unloadAll();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}
