#include "app/App.hpp"
#include "graphics/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace nuage {

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

    m_lastFrameTime = static_cast<float>(glfwGetTime());
    return true;
}

void App::run() {
    while (!m_shouldQuit && !glfwWindowShouldClose(m_window)) {
        float now = static_cast<float>(glfwGetTime());
        m_deltaTime = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_time = now;

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
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}
