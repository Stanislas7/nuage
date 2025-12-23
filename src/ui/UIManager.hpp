#pragma once

#include "ui/Text.hpp"
#include "ui/font/Font.hpp"
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

    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }

private:
    void buildTextVertexData(const Text& text, std::vector<float>& vertices, Vec3& pos) const;

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
};

}
