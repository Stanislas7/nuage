#include "ui/ui_manager.hpp"
#include "core/app.hpp"
#include "graphics/glad.h"
#include <GLFW/glfw3.h>
#include <array>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace nuage {

UIManager::~UIManager() {
    shutdown();
}

void UIManager::init() {
    if (!m_app) return;

    int width, height;
    glfwGetFramebufferSize(m_app->window(), &width, &height);
    m_windowWidth = width;
    m_windowHeight = height;
    m_projection = Mat4::ortho(0.0f, static_cast<float>(width),
                                static_cast<float>(height), 0.0f, -1.0f, 1.0f);

    m_font = std::make_unique<Font>();
    std::string fontPath = "assets/fonts/Roboto-Regular.ttf";
    std::ifstream testFile(fontPath);
    if (!testFile.good()) {
        std::cerr << "Font file not found: " << fontPath << std::endl;
        return;
    }
    testFile.close();

    if (!m_font->init(fontPath, 64.0f)) {
        std::cerr << "Failed to load font" << std::endl;
        return;
    }

    m_shader = m_app->assets().getShader("ui");
    if (!m_shader) {
        std::cerr << "UI shader not found in AssetStore" << std::endl;
        return;
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
}

void UIManager::shutdown() {
    m_texts.clear();
    m_buttons.clear();
    m_font.reset();
    m_shader = nullptr;

    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_whiteTexture) glDeleteTextures(1, &m_whiteTexture);

    m_vao = 0;
    m_vbo = 0;
    m_whiteTexture = 0;
}

void UIManager::update(double dt) {
    if (!m_app) return;
    int width, height;
    glfwGetFramebufferSize(m_app->window(), &width, &height);

    if (width != m_windowWidth || height != m_windowHeight) {
        m_windowWidth = width;
        m_windowHeight = height;
        m_projection = Mat4::ortho(0.0f, static_cast<float>(width),
                                    static_cast<float>(height), 0.0f, -1.0f, 1.0f);
    }

    Vec2 mousePos = m_app->input().mousePosition();
    bool mousePressed = m_app->input().isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);

    for (auto& btn : m_buttons) {
        if (!btn->visible || !btn->enabled) {
            btn->setHovered(false);
            continue;
        }

        Vec3 pos = btn->getAnchoredPosition(m_windowWidth, m_windowHeight);
        Vec3 size = btn->getSize();

        bool hovered = (mousePos.x >= pos.x && mousePos.x <= pos.x + size.x &&
                        mousePos.y >= pos.y && mousePos.y <= pos.y + size.y);
        
        btn->setHovered(hovered);

        if (hovered && mousePressed) {
            btn->triggerClick();
        }
    }
}

void UIManager::begin() {
    if (!m_shader || !m_font) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader->use();
    m_shader->setMat4("uProjection", m_projection);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
}

void UIManager::end() {
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void UIManager::drawPersistent() {
    // 1. Draw Buttons
    for (const auto& btn : m_buttons) {
        if (!btn->visible) continue;
        Vec3 fillColor = btn->isHovered() ? btn->getHoverColor() : btn->color;
        if (btn->isOutlineOnly()) {
            drawRoundedRect(btn->position.x, btn->position.y, btn->getSize().x, btn->getSize().y,
                            btn->getCornerRadius(), btn->getOutlineColor(), 1.0f, btn->anchor);

            float thickness = std::max(0.0f, btn->getOutlineThickness());
            float innerW = std::max(0.0f, btn->getSize().x - 2.0f * thickness);
            float innerH = std::max(0.0f, btn->getSize().y - 2.0f * thickness);
            if (innerW > 0.0f && innerH > 0.0f) {
                float xInset = 0.0f;
                float yInset = 0.0f;
                switch (btn->anchor) {
                    case Anchor::TopLeft:
                    case Anchor::TopRight:
                    case Anchor::BottomLeft:
                    case Anchor::BottomRight:
                        xInset = thickness;
                        yInset = thickness;
                        break;
                    case Anchor::Center:
                    default:
                        break;
                }

                drawRoundedRect(btn->position.x + xInset, btn->position.y + yInset, innerW, innerH,
                                std::max(0.0f, btn->getCornerRadius() - thickness),
                                fillColor, 1.0f, btn->anchor);
            }
        } else {
            drawRoundedRect(btn->position.x, btn->position.y, btn->getSize().x, btn->getSize().y,
                            btn->getCornerRadius(), fillColor, 1.0f, btn->anchor);
        }

        Text tempText(btn->getText(), m_font.get(), m_app);
        tempText.scaleVal(btn->scale);
        Vec3 textSize = tempText.getSize();
        Vec3 bPos = btn->getAnchoredPosition(m_windowWidth, m_windowHeight);
        float tx = bPos.x + (btn->getSize().x - textSize.x) / 2.0f;
        float ty = bPos.y + (btn->getSize().y - textSize.y) / 2.0f;

        drawText(btn->getText(), tx, ty, Anchor::TopLeft, btn->scale, Vec3(1, 1, 1), 1.0f);
    }

    // 2. Draw Texts
    for (const auto& text : m_texts) {
        if (!text || !text->visible) continue;
        if (text->getContent().empty()) continue;

        Vec3 pos = text->getAnchoredPosition(m_windowWidth, m_windowHeight);
        drawText(text->getContent(), pos.x, pos.y, Anchor::TopLeft, text->scale, text->color, 1.0f);
    }
}

void UIManager::drawRect(float x, float y, float w, float h, const Vec3& color, float alpha, Anchor anchor) {
    if (!m_shader) return;
    float rx = x;
    float ry = y;

    switch (anchor) {
        case Anchor::TopRight:    rx = m_windowWidth - x - w; break;
        case Anchor::BottomLeft:  ry = m_windowHeight - y - h; break;
        case Anchor::BottomRight: rx = m_windowWidth - x - w; ry = m_windowHeight - y - h; break;
        case Anchor::Center:      rx = m_windowWidth / 2.0f + x - w / 2.0f;
                                  ry = m_windowHeight / 2.0f + y - h / 2.0f; break;
        default: break;
    }

    const std::array<float, 24> verts = {
        rx,     ry,     0.0f, 0.0f,
        rx + w, ry,     1.0f, 0.0f,
        rx + w, ry + h, 1.0f, 1.0f,
        rx,     ry,     0.0f, 0.0f,
        rx + w, ry + h, 1.0f, 1.0f,
        rx,     ry + h, 0.0f, 1.0f
    };

    m_shader->setMat4("uModel", Mat4::identity());
    m_shader->setVec3("uColor", color);
    m_shader->setFloat("uAlpha", alpha);
    m_shader->setInt("uRounded", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    m_shader->setInt("uTexture", 0);
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(float), verts.data());
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void UIManager::drawRoundedRect(float x, float y, float w, float h, float radius, const Vec3& color, float alpha, Anchor anchor) {
    if (!m_shader) return;
    float rx = x;
    float ry = y;

    switch (anchor) {
        case Anchor::TopRight:    rx = m_windowWidth - x - w; break;
        case Anchor::BottomLeft:  ry = m_windowHeight - y - h; break;
        case Anchor::BottomRight: rx = m_windowWidth - x - w; ry = m_windowHeight - y - h; break;
        case Anchor::Center:      rx = m_windowWidth / 2.0f + x - w / 2.0f;
                                  ry = m_windowHeight / 2.0f + y - h / 2.0f; break;
        default: break;
    }

    const std::array<float, 24> verts = {
        rx,     ry,     0.0f, 0.0f,
        rx + w, ry,     1.0f, 0.0f,
        rx + w, ry + h, 1.0f, 1.0f,
        rx,     ry,     0.0f, 0.0f,
        rx + w, ry + h, 1.0f, 1.0f,
        rx,     ry + h, 0.0f, 1.0f
    };

    float maxRadius = 0.5f * std::min(w, h);
    float clampedRadius = std::clamp(radius, 0.0f, maxRadius);

    m_shader->setMat4("uModel", Mat4::identity());
    m_shader->setVec3("uColor", color);
    m_shader->setFloat("uAlpha", alpha);
    m_shader->setInt("uRounded", 1);
    m_shader->setVec2("uRectSize", Vec2(w, h));
    m_shader->setFloat("uRadius", clampedRadius);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    m_shader->setInt("uTexture", 0);

    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(float), verts.data());
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void UIManager::drawText(const std::string& content, float x, float y, Anchor anchor,
                         float scale, const Vec3& color, float alpha) {
    if (content.empty() || !m_font || !m_shader) return;

    Text text(content, m_font.get(), m_app);
    text.pos(x, y).scaleVal(scale).anchorMode(anchor);

    Vec3 pos = text.getAnchoredPosition(m_windowWidth, m_windowHeight);
    
    std::vector<float> vertices;
    buildTextVertexData(text, vertices, pos);
    if (vertices.empty()) return;

    m_shader->setMat4("uModel", Mat4::identity());
    m_shader->setVec3("uColor", color);
    m_shader->setFloat("uAlpha", std::clamp(alpha, 0.0f, 1.0f));
    m_shader->setInt("uRounded", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_font->getTexture());
    m_shader->setInt("uTexture", 0);

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    int quadCount = static_cast<int>(vertices.size()) / 24;
    glDrawArrays(GL_TRIANGLES, 0, quadCount * 6);
}

Text& UIManager::text(const std::string& content) {
    m_texts.push_back(std::make_unique<Text>(content, m_font.get(), m_app));
    return *m_texts.back();
}

Button& UIManager::button(const std::string& text) {
    m_buttons.push_back(std::make_unique<Button>(text, m_font.get(), m_app));
    return *m_buttons.back();
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
