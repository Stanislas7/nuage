#include "modules/render_module.hpp"
#include "core/simulator.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h> // Needed for basic GL definitions sometimes, but glad handles it mostly.

namespace flightsim {

void RenderModule::init(Simulator* sim) {
    Module::init(sim);
    glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, 1.0f);
}

void RenderModule::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

}
