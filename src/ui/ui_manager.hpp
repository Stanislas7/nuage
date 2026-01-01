#pragma once

#include "math/vec3.hpp"
#include "ui/anchor.hpp"
#include "ui/font/font.hpp"
#include "ui/text.hpp"
#include "ui/button.hpp"
#include "graphics/shader.hpp"
#include "math/mat4.hpp"
#include "core/subsystem.hpp"
#include "graphics/glad.h"
#include <vector>
#include <memory>
#include <string>

namespace nuage {

class App;
class Aircraft;
class PauseOverlay;
class DebugOverlay;
class HudOverlay;

struct UITheme {
    float defaultPadding = 20.0f;
    Vec3 buttonFill = Vec3(0.18f, 0.2f, 0.24f);
    Vec3 buttonHover = Vec3(0.24f, 0.28f, 0.34f);
    float buttonCornerRadius = 12.0f;
    float buttonTextScale = 0.7f;
};

class UIManager : public Subsystem {
public:
    UIManager() = default;
    ~UIManager();

    void setApp(App* app) { m_app = app; }
    void setAircraft(Aircraft* aircraft) { m_aircraft = aircraft; }

    // Subsystem interface
    void init() override;
    void update(double dt) override;
    void shutdown() override;
    std::string getName() const override { return "UI"; }
    std::vector<std::string> dependencies() const override { return {"AssetStore", "Input"}; }
    
    // Drawing Lifecycle
    void begin();
    void end();
    void render();
    void drawPersistent();

    // Factory methods
    Text& text(const std::string& content);
    Button& button(const std::string& text);
    
    // Primitives for custom overlays
    void drawRect(float x, float y, float w, float h, const Vec3& color, float alpha,
                  Anchor anchor = Anchor::TopLeft, bool applyPadding = true);
    void drawRoundedRect(float x, float y, float w, float h, float radius, const Vec3& color, float alpha,
                         Anchor anchor = Anchor::TopLeft, bool applyPadding = true);
    void drawText(const std::string& content, float x, float y, Anchor anchor = Anchor::TopLeft,
                  float scale = 1.0f, const Vec3& color = {1, 1, 1}, float alpha = 1.0f,
                  bool applyPadding = true);

    Vec3 measureText(const std::string& content, float scale = 1.0f) const;

    void setGroupVisible(const std::string& group, bool visible);
    void setGroupEnabled(const std::string& group, bool enabled);
    void setGroupActive(const std::string& group, bool active);

    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    App* app() { return m_app; }
    const UITheme& theme() const { return m_theme; }
    void setTheme(const UITheme& theme) { m_theme = theme; }

private:
    void buildTextVertexData(const Text& text, std::vector<float>& vertices, Vec3& pos) const;

    App* m_app = nullptr;
    Aircraft* m_aircraft = nullptr;
    std::unique_ptr<Font> m_font;
    Shader* m_shader = nullptr;
    
    // Overlays
    std::unique_ptr<PauseOverlay> m_pauseOverlay;
    std::unique_ptr<DebugOverlay> m_debugOverlay;
    std::unique_ptr<HudOverlay> m_hudOverlay;

    std::vector<std::unique_ptr<Text>> m_texts;
    std::vector<std::unique_ptr<Button>> m_buttons;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_whiteTexture = 0;

    Mat4 m_projection;
    UITheme m_theme;

    int m_windowWidth = 0;
    int m_windowHeight = 0;
};

}
