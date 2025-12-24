#include "ui/ui_manager.hpp"
#include "app/app.hpp"
#include "graphics/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>

namespace nuage {

UIManager::~UIManager() {
    shutdown();
}

bool UIManager::init(App* app) {
    m_app = app;

    int width, height;
    glfwGetFramebufferSize(app->window(), &width, &height);
    m_windowWidth = width;
    m_windowHeight = height;
    m_projection = Mat4::ortho(0.0f, static_cast<float>(width),
                                static_cast<float>(height), 0.0f, -1.0f, 1.0f);

    m_font = std::make_unique<Font>();
    std::string fontPath = "assets/fonts/Roboto-Regular.ttf";
    std::ifstream testFile(fontPath);
    if (!testFile.good()) {
        std::cerr << "Font file not found: " << fontPath << std::endl;
        return false;
    }
    testFile.close();

    if (!m_font->init(fontPath, 24.0f)) {
        std::cerr << "Failed to load font" << std::endl;
        return false;
    }

    const char* vertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoord;

        out vec2 vTexCoord;

        uniform mat4 uProjection;
        uniform mat4 uModel;

        void main() {
            vTexCoord = aTexCoord;
            gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
        }
    )";

    const char* fragmentSrc = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D uTexture;
        uniform vec3 uColor;

        void main() {
            float alpha = texture(uTexture, vTexCoord).r;
            FragColor = vec4(uColor, alpha);
        }
    )";

    m_shader = std::make_unique<Shader>();
    if (!m_shader->init(vertexSrc, fragmentSrc)) {
        std::cerr << "Failed to create UI shader" << std::endl;
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenTextures(1, &m_whiteTexture);

    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    unsigned char whitePixel[] = {255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

void UIManager::shutdown() {
    m_texts.clear();
    m_font.reset();
    m_shader.reset();

    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_whiteTexture) glDeleteTextures(1, &m_whiteTexture);

    m_vao = 0;
    m_vbo = 0;
    m_whiteTexture = 0;
}

void UIManager::update() {
    int width, height;
    glfwGetFramebufferSize(m_app->window(), &width, &height);

    if (width != m_windowWidth || height != m_windowHeight) {
        m_windowWidth = width;
        m_windowHeight = height;
        m_projection = Mat4::ortho(0.0f, static_cast<float>(width),
                                    static_cast<float>(height), 0.0f, -1.0f, 1.0f);
    }
}

void UIManager::draw() {
    if (!m_shader || !m_font) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader->use();
    m_shader->setMat4("uProjection", m_projection);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    for (const auto& text : m_texts) {
        Vec3 pos = text->getAnchoredPosition(m_windowWidth, m_windowHeight);

        std::vector<float> vertices;
        buildTextVertexData(*text, vertices, pos);

        if (vertices.empty()) continue;

        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

        m_shader->setMat4("uModel", Mat4::identity());
        m_shader->setVec3("uColor", text->color);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_font->getTexture());

        int quadCount = static_cast<int>(vertices.size()) / 16;
        glDrawArrays(GL_TRIANGLES, 0, quadCount * 6);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

Text& UIManager::text(const std::string& content) {
    m_texts.push_back(std::make_unique<Text>(content, m_font.get(), m_app));
    return *m_texts.back();
}

void UIManager::buildTextVertexData(const Text& text, std::vector<float>& vertices, Vec3& pos) const {
    if (!m_font) return;

    float x = pos.x;
    float y = pos.y + m_font->getAscent() * text.scale;

    for (char c : text.getContent()) {
        const GlyphInfo* glyph = m_font->getGlyph(c);
        if (!glyph) continue;

        float x0 = x + glyph->x0 * text.scale;
        float y0 = y + glyph->y0 * text.scale;
        float x1 = x + glyph->x1 * text.scale;
        float y1 = y + glyph->y1 * text.scale;

        float u0 = glyph->u0;
        float v0 = glyph->v0;
        float u1 = glyph->u1;
        float v1 = glyph->v1;

        vertices.insert(vertices.end(), {
            x0, y0, u0, v0,
            x1, y0, u1, v0,
            x0, y1, u0, v1,
            x1, y0, u1, v0,
            x1, y1, u1, v1,
            x0, y1, u0, v1
        });

        x += glyph->xAdvance * text.scale;
    }
}

}
