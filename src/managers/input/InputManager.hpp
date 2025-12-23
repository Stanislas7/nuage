#pragma once

struct GLFWwindow;

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

class InputManager {
public:
    void init(GLFWwindow* window);
    void update(float dt);

    const FlightInput& flight() const { return m_flight; }
    
    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;
    bool quitRequested() const { return m_quitRequested; }

private:
    void pollKeyboard();
    void mapToControls(float dt);

    GLFWwindow* m_window = nullptr;
    FlightInput m_flight;
    
    bool m_keys[512] = {};
    bool m_prevKeys[512] = {};
    bool m_quitRequested = false;
    float m_throttleAccum = 0.3f;
};

}
