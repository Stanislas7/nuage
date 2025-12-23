#include "ui/font/Font.hpp"
#include "math/vec3.hpp"
#include <fstream>
#include <iostream>

#define STB_TRUETYPE_IMPLEMENTATION
#include "ui/font/stb_truetype.h"

namespace nuage {

Font::~Font() {
    shutdown();
}

bool Font::init(const std::string& fontPath, float fontSize) {
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open font file: " << fontPath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> fontData(size);
    if (!file.read(reinterpret_cast<char*>(fontData.data()), size)) {
        std::cerr << "Failed to read font file" << std::endl;
        return false;
    }

    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontData.data(), 0)) {
        std::cerr << "Failed to initialize font" << std::endl;
        return false;
    }

    int atlasWidth = 1024;
    int atlasHeight = 1024;
    std::vector<unsigned char> atlas(atlasWidth * atlasHeight);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
    m_fontScale = scale;
    m_ascent = ascent * scale;
    m_descent = descent * scale;
    m_lineGap = lineGap * scale;

    stbtt_pack_context spc;
    if (!stbtt_PackBegin(&spc, atlas.data(), atlasWidth, atlasHeight, 0, 1, nullptr)) {
        std::cerr << "Failed to initialize font packing" << std::endl;
        return false;
    }

    stbtt_PackSetOversampling(&spc, 2, 2);

    std::vector<stbtt_packedchar> packedChars(95);
    if (!stbtt_PackFontRange(&spc, fontData.data(), 0, fontSize, 32, 95, packedChars.data())) {
        std::cerr << "Failed to pack font" << std::endl;
        stbtt_PackEnd(&spc);
        return false;
    }

    stbtt_PackEnd(&spc);

    for (int i = 0; i < 95; i++) {
        const auto& pc = packedChars[i];
        GlyphInfo glyph;

        glyph.u0 = static_cast<float>(pc.x0) / atlasWidth;
        glyph.v0 = static_cast<float>(pc.y0) / atlasHeight;
        glyph.u1 = static_cast<float>(pc.x1) / atlasWidth;
        glyph.v1 = static_cast<float>(pc.y1) / atlasHeight;

        glyph.x0 = pc.xoff;
        glyph.y0 = pc.yoff;
        glyph.x1 = pc.xoff2;
        glyph.y1 = pc.yoff2;
        glyph.xAdvance = pc.xadvance;

        m_glyphs[static_cast<char>(32 + i)] = glyph;
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return true;
}

void Font::shutdown() {
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    m_glyphs.clear();
}

const GlyphInfo* Font::getGlyph(char c) const {
    auto it = m_glyphs.find(c);
    if (it != m_glyphs.end()) {
        return &it->second;
    }
    auto spaceIt = m_glyphs.find(' ');
    if (spaceIt != m_glyphs.end()) {
        return &spaceIt->second;
    }
    return nullptr;
}

Vec3 Font::measureText(const std::string& text) const {
    float width = 0.0f;
    float height = m_ascent - m_descent;

    for (char c : text) {
        const GlyphInfo* glyph = getGlyph(c);
        if (glyph) {
            width += glyph->xAdvance;
        }
    }

    return Vec3(width, height, 0);
}

}
