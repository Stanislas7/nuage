#pragma once

#include "graphics/glad.h"
#include <string>
#include <unordered_map>

namespace nuage {

struct Vec3;

struct GlyphInfo {
    float u0, v0, u1, v1;
    float x0, y0, x1, y1;
    float xAdvance;
};

class Font {
public:
    Font() = default;
    ~Font();

    bool init(const std::string& fontPath, float fontSize);
    void shutdown();

    const GlyphInfo* getGlyph(char c) const;
    Vec3 measureText(const std::string& text) const;
    GLuint getTexture() const { return m_texture; }
    float getAscent() const { return m_ascent; }

private:
    GLuint m_texture = 0;
    std::unordered_map<char, GlyphInfo> m_glyphs;
    float m_fontScale = 1.0f;
    float m_ascent = 0.0f;
    float m_descent = 0.0f;
    float m_lineGap = 0.0f;
};

}
