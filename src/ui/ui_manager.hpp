#pragma once

#include "math/vec3.hpp"
#include "ui/anchor.hpp"
#include "ui/font/font.hpp"
#include "ui/text.hpp"
#include "graphics/shader.hpp"
#include "math/mat4.hpp"
#include <vector>
#include <memory>
#include <string>

namespace nuage {

class App;

class UIManager {
public:
    UIManager() = default;
    ~UIManager();

    bool init(App* app);
    void shutdown();
    void update();
    void draw();

    Text& text(const std::string& content);
    void setOverlay(bool active, const Vec3& color, float alpha);
    void setOverlayText(std::string title, std::string hint);

    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }

private:
    void buildTextVertexData(const Text& text, std::vector<float>& vertices, Vec3& pos) const;
    void drawImmediateText(const std::string& content, float x, float y, Anchor anchor,
                           float scale, const Vec3& color, float alpha);

    App* m_app = nullptr;
    std::unique_ptr<Font> m_font;
    std::unique_ptr<Shader> m_shader;
    std::vector<std::unique_ptr<Text>> m_texts;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_whiteTexture = 0;

    Mat4 m_projection;

    int m_windowWidth = 0;
    int m_windowHeight = 0;
    bool m_overlayActive = false;
    Vec3 m_overlayColor = Vec3(0, 0, 0);
    float m_overlayAlpha = 0.0f;
    std::string m_overlayTitle;
    std::string m_overlayHint;
};

}
