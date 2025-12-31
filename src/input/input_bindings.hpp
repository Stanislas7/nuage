#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct GLFWwindow;

namespace nuage {

struct AxisBinding {
    std::vector<int> positiveKeys;
    std::vector<int> negativeKeys;
    float scale = 1.0f;
};

class InputBindings {
public:
    void applyDefaults();
    bool loadFromConfig(GLFWwindow* window);

    const std::vector<int>& button(const std::string& name) const;
    const AxisBinding* axis(const std::string& name) const;

private:
    std::unordered_map<std::string, AxisBinding> m_axes;
    std::unordered_map<std::string, std::vector<int>> m_buttons;
};

}
