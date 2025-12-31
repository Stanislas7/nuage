#include "input/input.hpp"
#include "core/properties/property_bus.hpp"
#include "core/properties/property_paths.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <string>

namespace nuage {

void Input::setWindow(GLFWwindow* window) {
    m_window = window;
    glfwSetWindowUserPointer(window, this);
}

void Input::init() {
    loadBindingsFromConfig();
}

void Input::update(double dt) {
    if (!m_window) return;
    pollKeyboard();
    pollMouse();
    mapToControls(dt);
}

void Input::pollKeyboard() {
    std::copy(std::begin(m_keys), std::end(m_keys), std::begin(m_prevKeys));

    for (int key = 0; key < 512; key++) {
        m_keys[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    const auto& quitKeys = m_bindings.button("quit");
    if (!quitKeys.empty() && isKeyListDown(quitKeys)) {
        PropertyBus::global().set(Properties::Sim::QUIT_REQUESTED, true);
    }
}

void Input::mapToControls(double dt) {
    auto mapAxis = [&](const std::string& name) -> double {
        const AxisBinding* binding = m_bindings.axis(name);
        if (!binding) return 0.0;
        double value = 0.0;
        for (int key : binding->positiveKeys) {
            if (key >= 0 && key < 512 && m_keys[key]) {
                value += 1.0;
            }
        }
        for (int key : binding->negativeKeys) {
            if (key >= 0 && key < 512 && m_keys[key]) {
                value -= 1.0;
            }
        }
        return std::clamp(value, -1.0, 1.0) * binding->scale;
    };

    PropertyBus::global().set(Properties::Controls::ELEVATOR, mapAxis("pitch"));
    PropertyBus::global().set(Properties::Controls::RUDDER, mapAxis("yaw"));
    PropertyBus::global().set(Properties::Controls::AILERON, mapAxis("roll"));

    if (isKeyListDown(m_bindings.button("throttle_up"))) {
        m_throttleAccum = std::min(1.0f, m_throttleAccum + static_cast<float>(dt) * 0.5f);
    }
    if (isKeyListDown(m_bindings.button("throttle_down"))) {
        m_throttleAccum = std::max(0.0f, m_throttleAccum - static_cast<float>(dt) * 0.5f);
    }
    
    PropertyBus::global().set(Properties::Controls::THROTTLE, static_cast<double>(m_throttleAccum));

    bool braking = isKeyListDown(m_bindings.button("brake"));
    PropertyBus::global().set(Properties::Controls::BRAKE_LEFT, braking ? 1.0 : 0.0);
    PropertyBus::global().set(Properties::Controls::BRAKE_RIGHT, braking ? 1.0 : 0.0);

    if (isKeyPressed(GLFW_KEY_SPACE)) {
        bool current = PropertyBus::global().get(Properties::Sim::PAUSED, false);
        PropertyBus::global().set(Properties::Sim::PAUSED, !current);
    }

    if (isKeyPressed(GLFW_KEY_TAB)) {
        PropertyBus::global().set("sim/commands/toggle-camera", true);
    }

    if (isButtonPressed("debug_menu")) {
        bool current = PropertyBus::global().get(Properties::Sim::DEBUG_VISIBLE, false);
        PropertyBus::global().set(Properties::Sim::DEBUG_VISIBLE, !current);
    }
}

bool Input::isKeyDown(int key) const {
    return key < 512 && m_keys[key];
}

bool Input::isKeyPressed(int key) const {
    return key < 512 && m_keys[key] && !m_prevKeys[key];
}

bool Input::isButtonPressed(const std::string& name) const {
    const auto& keys = m_bindings.button(name);
    for (int key : keys) {
        if (key >= 0 && key < 512 && m_keys[key] && !m_prevKeys[key]) {
            return true;
        }
    }
    return false;
}

void Input::pollMouse() {
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    
    // scale mouse points to framebuffer pixels (high DPI / retina fix)
    int winW, winH, fbW, fbH;
    glfwGetWindowSize(m_window, &winW, &winH);
    glfwGetFramebufferSize(m_window, &fbW, &fbH);
    
    float scaleX = static_cast<float>(fbW) / winW;
    float scaleY = static_cast<float>(fbH) / winH;

    m_mousePos = Vec2(static_cast<float>(x) * scaleX, static_cast<float>(y) * scaleY);

    m_mouseDelta = m_mousePos - m_prevMousePos;
    m_prevMousePos = m_mousePos;

    for (int i = 0; i < 8; i++) {
        m_prevMouseButtons[i] = m_mouseButtons[i];
        m_mouseButtons[i] = glfwGetMouseButton(m_window, i) == GLFW_PRESS;
    }
}

bool Input::isMouseButtonDown(int button) const {
    return button < 8 && m_mouseButtons[button];
}

bool Input::isMouseButtonPressed(int button) const {
    return button < 8 && m_mouseButtons[button] && !m_prevMouseButtons[button];
}

void Input::setCursorMode(int mode) {
    glfwSetInputMode(m_window, GLFW_CURSOR, mode);
}

void Input::centerCursor() {
    int w, h;
    glfwGetWindowSize(m_window, &w, &h);
    glfwSetCursorPos(m_window, w / 2.0, h / 2.0);
}

void Input::loadBindingsFromConfig() {
    m_bindings.applyDefaults();
    m_bindings.loadFromConfig(m_window);
}

bool Input::isKeyListDown(const std::vector<int>& keys) const {
    for (int key : keys) {
        if (key >= 0 && key < 512 && m_keys[key]) {
            return true;
        }
    }
    return false;
}

}
