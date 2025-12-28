#pragma once

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include "math/vec2.hpp"
#include "core/subsystem.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

namespace nuage {

class App;

class Input : public Subsystem {
public:
    void setWindow(GLFWwindow* window);

    // Subsystem interface
    void init() override;
    void update(double dt) override;
    void shutdown() override {}
    std::string getName() const override { return "Input"; }

    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;
    bool isButtonPressed(const std::string& name) const;

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
    void mapToControls(double dt);

    GLFWwindow* m_window = nullptr;

    bool m_keys[512] = {};
    bool m_prevKeys[512] = {};
    float m_throttleAccum = 0.3f;

    Vec2 m_mousePos{0, 0};
    Vec2 m_prevMousePos{0, 0};
    Vec2 m_mouseDelta{0, 0};

    bool m_mouseButtons[8] = {};
    bool m_prevMouseButtons[8] = {};
};

}
