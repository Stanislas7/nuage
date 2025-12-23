#include "core/simulator.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace flightsim {

bool Simulator::init(const SimulatorConfig& config) {
    // Init GLFW
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

    // Init all modules
    initModules();

    m_lastFrameTime = glfwGetTime();
    return true;
}

void Simulator::initModules() {
    m_input.init(this);
    m_aircraft.init(this);
    m_terrain.init(this);
    m_camera.init(this);
    m_render.init(this);
}

void Simulator::run() {
    while (!shouldClose()) {
        float currentTime = glfwGetTime();
        m_deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
        m_time = currentTime;

        tick(m_deltaTime);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Simulator::tick(float dt) {
    // UPDATE PHASE - modify state
    m_input.update(dt);
    m_aircraft.update(dt);
    m_terrain.update(dt);
    m_camera.update(dt);

    // RENDER PHASE - draw to screen
    m_render.render();      // Clear, setup state
    m_terrain.render();
    m_aircraft.render();
}

bool Simulator::shouldClose() const {
    return m_shouldQuit || glfwWindowShouldClose(m_window);
}

void Simulator::shutdown() {
    shutdownModules();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Simulator::shutdownModules() {
    m_render.shutdown();
    m_camera.shutdown();
    m_terrain.shutdown();
    m_aircraft.shutdown();
    m_input.shutdown();
}

}
