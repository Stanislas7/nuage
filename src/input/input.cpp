#include "input/input.hpp"
#include "utils/config_loader.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nuage {

namespace {
    using json = nuage::json;

    struct AxisBinding {
        std::vector<int> positiveKeys;
        std::vector<int> negativeKeys;
        float scale = 1.0f;
    };

    struct LayoutMatch {
        std::string name;
        const json* entry = nullptr;
    };

    std::unordered_map<std::string, AxisBinding> g_axisBindings;
    std::unordered_map<std::string, std::vector<int>> g_buttonBindings;

    constexpr const char* kControlsConfigPath = "assets/config/controls.json";
    constexpr const char* kLayoutsConfigPath = "assets/config/layouts.json";

    bool equalsIgnoreCase(const std::string& lhs, const std::string& rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (size_t i = 0; i < lhs.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
                std::tolower(static_cast<unsigned char>(rhs[i]))) {
                return false;
            }
        }
        return true;
    }

    std::string normalizeKeyName(const std::string& raw) {
        std::string normalized;
        normalized.reserve(raw.size());
        for (char c : raw) {
            if (c == ' ' || c == '-') continue;
            normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
        return normalized;
    }

    int keyFromNormalizedName(const std::string& normalized) {
        if (normalized.empty()) return -1;
        if (normalized.size() == 1 && std::isalpha(static_cast<unsigned char>(normalized[0]))) {
            return GLFW_KEY_A + (normalized[0] - 'A');
        }
        static const std::unordered_map<std::string, int> kSpecialKeys = {
            {"SPACE", GLFW_KEY_SPACE},
            {"SPACEBAR", GLFW_KEY_SPACE},
            {"LEFTSHIFT", GLFW_KEY_LEFT_SHIFT},
            {"RIGHTSHIFT", GLFW_KEY_RIGHT_SHIFT},
            {"LSHIFT", GLFW_KEY_LEFT_SHIFT},
            {"RSHIFT", GLFW_KEY_RIGHT_SHIFT},
            {"SHIFT", GLFW_KEY_LEFT_SHIFT},
            {"LEFTCONTROL", GLFW_KEY_LEFT_CONTROL},
            {"RIGHTCONTROL", GLFW_KEY_RIGHT_CONTROL},
            {"CONTROL", GLFW_KEY_LEFT_CONTROL},
            {"CTRL", GLFW_KEY_LEFT_CONTROL},
            {"LEFTALT", GLFW_KEY_LEFT_ALT},
            {"RIGHTALT", GLFW_KEY_RIGHT_ALT},
            {"ALT", GLFW_KEY_LEFT_ALT},
            {"TAB", GLFW_KEY_TAB},
            {"ESCAPE", GLFW_KEY_ESCAPE},
            {"ESC", GLFW_KEY_ESCAPE},
            {"ENTER", GLFW_KEY_ENTER},
            {"RETURN", GLFW_KEY_ENTER},
            {"BACKSPACE", GLFW_KEY_BACKSPACE},
            {"CAPSLOCK", GLFW_KEY_CAPS_LOCK},
            {"CAPS", GLFW_KEY_CAPS_LOCK},
            {"GRAVE", GLFW_KEY_GRAVE_ACCENT},
            {"MINUS", GLFW_KEY_MINUS},
            {"EQUAL", GLFW_KEY_EQUAL},
            {"LEFTBRACKET", GLFW_KEY_LEFT_BRACKET},
            {"RIGHTBRACKET", GLFW_KEY_RIGHT_BRACKET},
            {"SEMICOLON", GLFW_KEY_SEMICOLON},
            {"APOSTROPHE", GLFW_KEY_APOSTROPHE},
            {"COMMA", GLFW_KEY_COMMA},
            {"PERIOD", GLFW_KEY_PERIOD},
            {"DOT", GLFW_KEY_PERIOD},
            {"SLASH", GLFW_KEY_SLASH},
            {"BACKSLASH", GLFW_KEY_BACKSLASH}
        };
        static const std::unordered_map<std::string, int> kArrowKeys = {
            {"UP", GLFW_KEY_UP},
            {"ARROWUP", GLFW_KEY_UP},
            {"DOWN", GLFW_KEY_DOWN},
            {"ARROWDOWN", GLFW_KEY_DOWN},
            {"LEFT", GLFW_KEY_LEFT},
            {"ARROWLEFT", GLFW_KEY_LEFT},
            {"RIGHT", GLFW_KEY_RIGHT},
            {"ARROWRIGHT", GLFW_KEY_RIGHT}
        };
        static const std::unordered_map<std::string, int> kExtraKeys = {
            {"PAGEUP", GLFW_KEY_PAGE_UP},
            {"PGUP", GLFW_KEY_PAGE_UP},
            {"PAGEDOWN", GLFW_KEY_PAGE_DOWN},
            {"PGDN", GLFW_KEY_PAGE_DOWN},
            {"KEYPADADD", GLFW_KEY_KP_ADD},
            {"KEYPADPLUS", GLFW_KEY_KP_ADD},
            {"KEYPADSUBTRACT", GLFW_KEY_KP_SUBTRACT},
            {"KEYPADMINUS", GLFW_KEY_KP_SUBTRACT},
            {"KEYPAD-", GLFW_KEY_KP_SUBTRACT},
            {"KEYPAD_", GLFW_KEY_KP_SUBTRACT},
            {"KEYPAD/", GLFW_KEY_KP_DIVIDE},
            {"KEYPAD*", GLFW_KEY_KP_MULTIPLY},
            {"KEYPADENTER", GLFW_KEY_KP_ENTER},
            {"KEYPAD0", GLFW_KEY_KP_0}
        };
        auto it = kSpecialKeys.find(normalized);
        if (it != kSpecialKeys.end()) {
            return it->second;
        }
        auto extraIt = kExtraKeys.find(normalized);
        if (extraIt != kExtraKeys.end()) {
            return extraIt->second;
        }
        auto arrowIt = kArrowKeys.find(normalized);
        if (arrowIt != kArrowKeys.end()) {
            return arrowIt->second;
        }
        return -1;
    }

    int keyFromName(const std::string& rawName,
                    const std::unordered_map<std::string, int>* layoutMap = nullptr) {
        std::string normalized = normalizeKeyName(rawName);
        if (normalized.empty()) return -1;
        if (layoutMap) {
            auto it = layoutMap->find(normalized);
            if (it != layoutMap->end()) {
                return it->second;
            }
        }
        return keyFromNormalizedName(normalized);
    }

    std::vector<int> parseKeyList(const json& value, const std::string& context,
                                  const std::unordered_map<std::string, int>* layoutMap) {
        std::vector<int> keys;
        if (value.is_null()) return keys;
        auto addKey = [&](const std::string& raw) {
            int key = keyFromName(raw, layoutMap);
            if (key >= 0) {
                keys.push_back(key);
            } else {
                std::cerr << "Unknown key \"" << raw << "\" in " << context << std::endl;
            }
        };
        if (value.is_string()) {
            addKey(value.get<std::string>());
        } else if (value.is_array()) {
            for (const auto& entry : value) {
                if (!entry.is_string()) continue;
                addKey(entry.get<std::string>());
            }
        } else if (!value.is_object()) {
            std::cerr << "Unexpected key value for " << context << std::endl;
        }
        return keys;
    }

    const std::vector<int>& getButtonBinding(const std::string& name) {
        static const std::vector<int> kEmpty;
        auto it = g_buttonBindings.find(name);
        return it != g_buttonBindings.end() ? it->second : kEmpty;
    }

    void applyDefaultBindings() {
        g_axisBindings.clear();
        g_buttonBindings.clear();
        g_axisBindings["pitch"] = {{GLFW_KEY_W}, {GLFW_KEY_S}};
        g_axisBindings["roll"] = {{GLFW_KEY_D}, {GLFW_KEY_A}};
        g_axisBindings["yaw"] = {{GLFW_KEY_E}, {GLFW_KEY_Q}};
        g_buttonBindings["throttle_up"] = {GLFW_KEY_SPACE};
        g_buttonBindings["throttle_down"] = {GLFW_KEY_LEFT_SHIFT};
        g_buttonBindings["brake"] = {GLFW_KEY_B};
        g_buttonBindings["quit"] = {GLFW_KEY_ESCAPE};
    }

    bool applyConfigBindings(const json& controls,
                             const std::unordered_map<std::string, int>* layoutMap) {
        bool appliedAny = false;
        if (controls.contains("axes") && controls["axes"].is_object()) {
            for (const auto& axisItem : controls["axes"].items()) {
                const auto& axisData = axisItem.value();
                const std::string context = "axes." + axisItem.key();
                const json positiveValue = axisData.contains("positive") ? axisData["positive"] : json();
                const json negativeValue = axisData.contains("negative") ? axisData["negative"] : json();
                AxisBinding binding;
                binding.positiveKeys = parseKeyList(positiveValue, context + ".positive", layoutMap);
                binding.negativeKeys = parseKeyList(negativeValue, context + ".negative", layoutMap);
                float scale = axisData.value("scale", 1.0f);
                if (axisData.value("invert", false)) {
                    scale = -scale;
                }
                binding.scale = scale;
                if (!binding.positiveKeys.empty() || !binding.negativeKeys.empty()) {
                    g_axisBindings[axisItem.key()] = std::move(binding);
                    appliedAny = true;
                }
            }
        }
        if (controls.contains("buttons") && controls["buttons"].is_object()) {
            for (const auto& buttonItem : controls["buttons"].items()) {
                const std::string context = "buttons." + buttonItem.key();
                auto keys = parseKeyList(buttonItem.value(), context, layoutMap);
                if (!keys.empty()) {
                    g_buttonBindings[buttonItem.key()] = std::move(keys);
                    appliedAny = true;
                }
            }
        }
        return appliedAny;
    }

    void applyLayoutOverrides(json& controls, const json& overrides) {
        if (overrides.contains("axes") && overrides["axes"].is_object()) {
            for (const auto& axisItem : overrides["axes"].items()) {
                auto& target = controls["axes"][axisItem.key()];
                if (!target.is_object()) {
                    target = json::object();
                }
                for (const auto& field : axisItem.value().items()) {
                    target[field.key()] = field.value();
                }
            }
        }
        if (overrides.contains("buttons") && overrides["buttons"].is_object()) {
            for (const auto& buttonItem : overrides["buttons"].items()) {
                controls["buttons"][buttonItem.key()] = buttonItem.value();
            }
        }
    }

    std::unordered_map<std::string, int> buildLayoutKeyMap(const json& layoutEntry) {
        std::unordered_map<std::string, int> map;
        auto addMapping = [&](const std::string& character, const std::string& physical) {
            std::string normalizedChar = normalizeKeyName(character);
            if (normalizedChar.empty()) return;
            int physicalKey = keyFromName(physical, nullptr);
            if (physicalKey >= 0) {
                map[normalizedChar] = physicalKey;
            }
        };
        if (layoutEntry.contains("mapping") && layoutEntry["mapping"].is_object()) {
            for (const auto& entry : layoutEntry["mapping"].items()) {
                if (!entry.value().is_string()) continue;
                addMapping(entry.key(), entry.value().get<std::string>());
            }
        }
        if (layoutEntry.contains("detect") && layoutEntry["detect"].is_object()) {
            for (const auto& entry : layoutEntry["detect"].items()) {
                if (!entry.value().is_string()) continue;
                addMapping(entry.value().get<std::string>(), entry.key());
            }
        }
        return map;
    }

    LayoutMatch detectKeyboardLayout(GLFWwindow* window, const json& layoutRoot) {
        if (!layoutRoot.contains("layouts") || !layoutRoot["layouts"].is_array()) {
            return {"qwerty", nullptr};
        }
        const auto& layouts = layoutRoot["layouts"];
        LayoutMatch fallback{"qwerty", nullptr};
        for (const auto& layout : layouts) {
            if (!layout.is_object()) continue;
            const std::string name = layout.value("name", std::string{});
            if (name.empty()) continue;
            if (!fallback.entry) {
                fallback.name = name;
                fallback.entry = &layout;
            }
            if (layout.value("default", false)) {
                fallback.name = name;
                fallback.entry = &layout;
            }
            const auto& detect = layout.value("detect", json::object());
            if (!detect.is_object() || detect.empty()) {
                continue;
            }
            bool matches = true;
            for (const auto& entry : detect.items()) {
                int key = keyFromName(entry.key());
                if (key < 0) {
                    matches = false;
                    break;
                }
                int scancode = glfwGetKeyScancode(key);
                const char* actual = glfwGetKeyName(key, scancode);
                if (!actual || !equalsIgnoreCase(actual, entry.value().get<std::string>())) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                return {name, &layout};
            }
        }
        return fallback;
    }
}

void Input::init(GLFWwindow* window) {
    m_window = window;
    glfwSetWindowUserPointer(window, this);
    loadBindingsFromConfig();
}

void Input::update(float dt) {
    pollKeyboard();
    pollMouse();
    mapToControls(dt);
}

void Input::pollKeyboard() {
    std::copy(std::begin(m_keys), std::end(m_keys), std::begin(m_prevKeys));

    for (int key = 0; key < 512; key++) {
        m_keys[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    auto quitIt = g_buttonBindings.find("quit");
    if (quitIt != g_buttonBindings.end() && isKeyListDown(quitIt->second)) {
        m_quitRequested = true;
    }
}

void Input::mapToControls(float dt) {
    auto mapAxis = [&](const std::string& name) -> float {
        auto it = g_axisBindings.find(name);
        if (it == g_axisBindings.end()) return 0.0f;
        float value = 0.0f;
        for (int key : it->second.positiveKeys) {
            if (key >= 0 && key < 512 && m_keys[key]) {
                value += 1.0f;
            }
        }
        for (int key : it->second.negativeKeys) {
            if (key >= 0 && key < 512 && m_keys[key]) {
                value -= 1.0f;
            }
        }
        return std::clamp(value, -1.0f, 1.0f) * it->second.scale;
    };

    m_flight.pitch = mapAxis("pitch");
    m_flight.yaw = mapAxis("yaw");
    m_flight.roll = mapAxis("roll");

    if (isKeyListDown(getButtonBinding("throttle_up"))) {
        m_throttleAccum = std::min(1.0f, m_throttleAccum + dt * 0.5f);
    }
    if (isKeyListDown(getButtonBinding("throttle_down"))) {
        m_throttleAccum = std::max(0.0f, m_throttleAccum - dt * 0.5f);
    }
    m_flight.throttle = m_throttleAccum;

    m_flight.brake = isKeyListDown(getButtonBinding("brake"));
}

bool Input::isKeyDown(int key) const {
    return key < 512 && m_keys[key];
}

bool Input::isKeyPressed(int key) const {
    return key < 512 && m_keys[key] && !m_prevKeys[key];
}

void Input::pollMouse() {
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
    applyDefaultBindings();

    auto controlsOpt = loadJsonConfig(kControlsConfigPath);
    if (!controlsOpt) {
        std::cerr << "Failed to load controls config \"" << kControlsConfigPath << "\", using defaults\n";
        return;
    }

    nuage::json controls = *controlsOpt;
    auto layoutOpt = loadJsonConfig(kLayoutsConfigPath);
    LayoutMatch layoutMatch{"qwerty", nullptr};
    if (!layoutOpt) {
        std::cerr << "Failed to load layout config \"" << kLayoutsConfigPath << "\", skipping detection\n";
    } else {
        glfwPollEvents();  // ensure GLFW has up-to-date key name data for layout detection
        layoutMatch = detectKeyboardLayout(m_window, *layoutOpt);
        if (layoutMatch.entry && layoutMatch.entry->contains("controls")) {
            applyLayoutOverrides(controls, (*layoutMatch.entry)["controls"]);
        }
    }

    std::unordered_map<std::string, int> layoutMap;
    if (layoutMatch.entry) {
        layoutMap = buildLayoutKeyMap(*layoutMatch.entry);
    }
    const std::unordered_map<std::string, int>* layoutMapPtr =
        layoutMatch.entry ? &layoutMap : nullptr;
    applyConfigBindings(controls, layoutMapPtr);
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
