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
    glfwSetWindowUserPointer(window, this);

    g_axisBindings["pitch"] = {GLFW_KEY_W, GLFW_KEY_S};  // AZERTY Z/S -> QWERTY W/S
    g_axisBindings["roll"] = {GLFW_KEY_D, GLFW_KEY_A};   // AZERTY D/Q -> QWERTY D/A
    g_axisBindings["yaw"] = {GLFW_KEY_E, GLFW_KEY_Q};    // AZERTY E/A -> QWERTY E/Q

    g_keyBindings["throttle_up"] = GLFW_KEY_SPACE;
    g_keyBindings["throttle_down"] = GLFW_KEY_LEFT_SHIFT;
    g_keyBindings["brake"] = GLFW_KEY_B;
    g_keyBindings["quit"] = GLFW_KEY_ESCAPE;
}

void InputManager::update(float dt) {
    pollKeyboard();
    pollMouse();
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

void InputManager::pollMouse() {
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    m_mousePos = Vec2(static_cast<float>(x), static_cast<float>(y));

    m_mouseDelta = m_mousePos - m_prevMousePos;
    m_prevMousePos = m_mousePos;

    for (int i = 0; i < 8; i++) {
        m_prevMouseButtons[i] = m_mouseButtons[i];
        m_mouseButtons[i] = glfwGetMouseButton(m_window, i) == GLFW_PRESS;
    }
}

bool InputManager::isMouseButtonDown(int button) const {
    return button < 8 && m_mouseButtons[button];
}

bool InputManager::isMouseButtonPressed(int button) const {
    return button < 8 && m_mouseButtons[button] && !m_prevMouseButtons[button];
}

void InputManager::setCursorMode(int mode) {
    glfwSetInputMode(m_window, GLFW_CURSOR, mode);
}

void InputManager::centerCursor() {
    int w, h;
    glfwGetWindowSize(m_window, &w, &h);
    glfwSetCursorPos(m_window, w / 2.0, h / 2.0);
}

}
