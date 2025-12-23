#include "managers/input/InputManager.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <unordered_map>
#include <string>

namespace nuage {

namespace {
    struct AxisBinding {
        int positiveKey;
        int negativeKey;
    };
    
    std::unordered_map<std::string, AxisBinding> g_axisBindings;
    std::unordered_map<std::string, int> g_keyBindings;
}

void InputManager::init(GLFWwindow* window) {
    m_window = window;
    
    g_axisBindings["pitch"] = {GLFW_KEY_UP, GLFW_KEY_DOWN};
    g_axisBindings["yaw"] = {GLFW_KEY_Q, GLFW_KEY_E};
    g_axisBindings["roll"] = {GLFW_KEY_LEFT, GLFW_KEY_RIGHT};

    g_keyBindings["throttle_up"] = GLFW_KEY_SPACE;
    g_keyBindings["throttle_down"] = GLFW_KEY_LEFT_SHIFT;
    g_keyBindings["brake"] = GLFW_KEY_B;
    g_keyBindings["quit"] = GLFW_KEY_ESCAPE;
}

void InputManager::update(float dt) {
    pollKeyboard();
    mapToControls(dt);
}

void InputManager::pollKeyboard() {
    std::copy(std::begin(m_keys), std::end(m_keys), std::begin(m_prevKeys));

    for (int key = 0; key < 512; key++) {
        m_keys[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    if (m_keys[g_keyBindings["quit"]]) {
        m_quitRequested = true;
    }
}

void InputManager::mapToControls(float dt) {
    auto mapAxis = [&](const std::string& name) -> float {
        auto it = g_axisBindings.find(name);
        if (it == g_axisBindings.end()) return 0.0f;
        float value = 0.0f;
        if (m_keys[it->second.positiveKey]) value += 1.0f;
        if (m_keys[it->second.negativeKey]) value -= 1.0f;
        return value;
    };

    m_flight.pitch = mapAxis("pitch");
    m_flight.yaw = mapAxis("yaw");
    m_flight.roll = mapAxis("roll");

    if (m_keys[g_keyBindings["throttle_up"]]) {
        m_throttleAccum = std::min(1.0f, m_throttleAccum + dt * 0.5f);
    }
    if (m_keys[g_keyBindings["throttle_down"]]) {
        m_throttleAccum = std::max(0.0f, m_throttleAccum - dt * 0.5f);
    }
    m_flight.throttle = m_throttleAccum;

    m_flight.brake = m_keys[g_keyBindings["brake"]];
}

bool InputManager::isKeyDown(int key) const {
    return key < 512 && m_keys[key];
}

bool InputManager::isKeyPressed(int key) const {
    return key < 512 && m_keys[key] && !m_prevKeys[key];
}

}
