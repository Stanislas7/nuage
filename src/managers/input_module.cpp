#include "managers/input_module.hpp"
#include "app/simulator.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>

namespace flightsim {

void InputModule::init(Simulator* sim) {
    Module::init(sim);
    m_window = sim->window();

    // Default bindings
    m_axisBindings["pitch"] = {GLFW_KEY_UP, GLFW_KEY_DOWN};
    m_axisBindings["yaw"] = {GLFW_KEY_Q, GLFW_KEY_E};
    m_axisBindings["roll"] = {GLFW_KEY_LEFT, GLFW_KEY_RIGHT};

    m_keyBindings["throttle_up"] = GLFW_KEY_SPACE;
    m_keyBindings["throttle_down"] = GLFW_KEY_LEFT_SHIFT;
    m_keyBindings["brake"] = GLFW_KEY_B;
    m_keyBindings["quit"] = GLFW_KEY_ESCAPE;
}

void InputModule::update(float dt) {
    pollKeyboard();
    mapToControls(dt);
}

void InputModule::pollKeyboard() {
    // Save previous state
    std::copy(std::begin(m_keys), std::end(m_keys), std::begin(m_prevKeys));

    // Poll current state
    for (int key = 0; key < 512; key++) {
        m_keys[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    // Check quit
    if (m_keys[m_keyBindings["quit"]]) {
        m_quitRequested = true;
    }
}

void InputModule::mapToControls(float dt) {
    auto mapAxis = [&](const std::string& name) -> float {
        auto it = m_axisBindings.find(name);
        if (it == m_axisBindings.end()) return 0.0f;
        float value = 0.0f;
        if (m_keys[it->second.positiveKey]) value += 1.0f;
        if (m_keys[it->second.negativeKey]) value -= 1.0f;
        return value;
    };

    m_controls.pitch = mapAxis("pitch");
    m_controls.yaw = mapAxis("yaw");
    m_controls.roll = mapAxis("roll");

    // Throttle accumulates over time
    if (m_keys[m_keyBindings["throttle_up"]]) {
        m_currentThrottle = std::min(1.0f, m_currentThrottle + dt * 0.5f);
    }
    if (m_keys[m_keyBindings["throttle_down"]]) {
        m_currentThrottle = std::max(0.0f, m_currentThrottle - dt * 0.5f);
    }
    m_controls.throttle = m_currentThrottle;

    m_controls.brake = m_keys[m_keyBindings["brake"]];
}

bool InputModule::isKeyDown(int key) const {
    return key < 512 && m_keys[key];
}

bool InputModule::isKeyPressed(int key) const {
    return key < 512 && m_keys[key] && !m_prevKeys[key];
}

}
