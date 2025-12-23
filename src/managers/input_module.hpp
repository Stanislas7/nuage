#pragma once

#include "managers/module.hpp"
#include <unordered_map>
#include <string>

struct GLFWwindow;

namespace flightsim {

// Logical flight controls - what the pilot wants
struct FlightControls {
    float pitch = 0.0f;      // -1 (nose down) to +1 (nose up)
    float yaw = 0.0f;        // -1 (left) to +1 (right)
    float roll = 0.0f;       // -1 (left) to +1 (right)
    float throttle = 0.0f;   // 0 to 1
    bool brake = false;
};

class InputModule : public Module {
public:
    const char* name() const override { return "Input"; }

    void init(Simulator* sim) override;
    void update(float dt) override;

    // Access current controls
    const FlightControls& controls() const { return m_controls; }

    // Queries
    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;  // Just pressed this frame
    bool quitRequested() const { return m_quitRequested; }

    // Rebinding
    void loadBindings(const std::string& path);

private:
    void pollKeyboard();
    void mapToControls(float dt);

    GLFWwindow* m_window = nullptr;
    FlightControls m_controls;

    bool m_keys[512] = {};
    bool m_prevKeys[512] = {};
    bool m_quitRequested = false;

    float m_currentThrottle = 0.3f;  // Throttle accumulates

    // Key bindings (key code -> action name)
    struct AxisBinding {
        int positiveKey;
        int negativeKey;
    };
    std::unordered_map<std::string, AxisBinding> m_axisBindings;
    std::unordered_map<std::string, int> m_keyBindings;
};

}
