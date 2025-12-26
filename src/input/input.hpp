#pragma once

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include "math/vec2.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

namespace nuage {

struct FlightInput {
    float pitch = 0.0f;      // -1 to 1
    float roll = 0.0f;       // -1 to 1
    float yaw = 0.0f;        // -1 to 1
    float throttle = 0.0f;   // 0 to 1
    bool toggleGear = false;
    bool toggleFlaps = false;
    bool brake = false;
};

class App;

class Input {
public:
    void init(GLFWwindow* window);
    void update(float dt);

    const FlightInput& flight() const { return m_flight; }

    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;
    bool quitRequested() const { return m_quitRequested; }

    Vec2 mousePosition() const { return m_mousePos; }
    Vec2 mouseDelta() const { return m_mouseDelta; }
    bool isMouseButtonDown(int button) const;
    bool isMouseButtonPressed(int button) const;

    void setCursorMode(int mode);
    void centerCursor();

    void loadBindingsFromConfig();
    bool isKeyListDown(const std::vector<int>& keys) const;

private:
    void pollKeyboard();
    void pollMouse();
    void mapToControls(float dt);

    GLFWwindow* m_window = nullptr;
    FlightInput m_flight;

    bool m_keys[512] = {};
    bool m_prevKeys[512] = {};
    bool m_quitRequested = false;
    float m_throttleAccum = 0.3f;

    Vec2 m_mousePos{0, 0};
    Vec2 m_prevMousePos{0, 0};
    Vec2 m_mouseDelta{0, 0};

    bool m_mouseButtons[8] = {};
    bool m_prevMouseButtons[8] = {};
};

}
