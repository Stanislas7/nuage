#pragma once

#include "math/vec3.hpp"
#include "ui/anchor.hpp"
#include "ui/font/font.hpp"
#include "ui/text.hpp"
#include "ui/button.hpp"
#include "graphics/shader.hpp"
#include "math/mat4.hpp"
#include "core/subsystem.hpp"
#include <vector>
#include <memory>
#include <string>

namespace nuage {

class App;

class UIManager : public Subsystem {
public:
    UIManager() = default;
    ~UIManager();

    void setApp(App* app) { m_app = app; }

    // Subsystem interface
    void init() override;
    void update(double dt) override;
    void shutdown() override;
    std::string getName() const override { return "UI"; }
    
    // Drawing Lifecycle
    void begin();
    void end();
    void drawPersistent();

    // Factory methods
    Text& text(const std::string& content);
    Button& button(const std::string& text);
    
    // Primitives for custom overlays
    void drawRect(float x, float y, float w, float h, const Vec3& color, float alpha, Anchor anchor = Anchor::TopLeft);
    void drawRoundedRect(float x, float y, float w, float h, float radius, const Vec3& color, float alpha, Anchor anchor = Anchor::TopLeft);
    void drawText(const std::string& content, float x, float y, Anchor anchor = Anchor::TopLeft,
                  float scale = 1.0f, const Vec3& color = {1, 1, 1}, float alpha = 1.0f);

    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    App* app() { return m_app; }

private:
    void buildTextVertexData(const Text& text, std::vector<float>& vertices, Vec3& pos) const;

    App* m_app = nullptr;
    std::unique_ptr<Font> m_font;
    Shader* m_shader = nullptr;
    std::vector<std::unique_ptr<Text>> m_texts;
    std::vector<std::unique_ptr<Button>> m_buttons;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_whiteTexture = 0;

    Mat4 m_projection;

    int m_windowWidth = 0;
    int m_windowHeight = 0;
};

}
