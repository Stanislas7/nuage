#include "graphics/texture_array.hpp"
#include "utils/stb_image.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace nuage {
namespace {

std::vector<unsigned char> resampleRGBA(const unsigned char* src, int srcW, int srcH,
                                        int dstW, int dstH) {
    std::vector<unsigned char> out(static_cast<size_t>(dstW * dstH * 4));
    for (int y = 0; y < dstH; ++y) {
        float v = (dstH > 1) ? static_cast<float>(y) / (dstH - 1) : 0.0f;
        float srcY = v * (srcH - 1);
        int y0 = static_cast<int>(std::floor(srcY));
        int y1 = std::min(y0 + 1, srcH - 1);
        float fy = srcY - static_cast<float>(y0);
        for (int x = 0; x < dstW; ++x) {
            float u = (dstW > 1) ? static_cast<float>(x) / (dstW - 1) : 0.0f;
            float srcX = u * (srcW - 1);
            int x0 = static_cast<int>(std::floor(srcX));
            int x1 = std::min(x0 + 1, srcW - 1);
            float fx = srcX - static_cast<float>(x0);
            for (int c = 0; c < 4; ++c) {
                int idx00 = (y0 * srcW + x0) * 4 + c;
                int idx10 = (y0 * srcW + x1) * 4 + c;
                int idx01 = (y1 * srcW + x0) * 4 + c;
                int idx11 = (y1 * srcW + x1) * 4 + c;
                float v00 = static_cast<float>(src[idx00]);
                float v10 = static_cast<float>(src[idx10]);
                float v01 = static_cast<float>(src[idx01]);
                float v11 = static_cast<float>(src[idx11]);
                float v0 = v00 + (v10 - v00) * fx;
                float v1 = v01 + (v11 - v01) * fx;
                float vFinal = v0 + (v1 - v0) * fy;
                out[static_cast<size_t>(y * dstW + x) * 4 + c] = static_cast<unsigned char>(vFinal);
            }
        }
    }
    return out;
}

} // namespace

TextureArray::~TextureArray() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
    }
}

bool TextureArray::loadFromFiles(const std::vector<std::string>& paths, int targetSize, bool repeat) {
    if (paths.empty() || targetSize <= 0) {
        return false;
    }
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    m_layers = static_cast<int>(paths.size());
    m_size = targetSize;

    std::vector<std::vector<unsigned char>> layers;
    layers.reserve(paths.size());

    stbi_set_flip_vertically_on_load(1);

    for (const auto& path : paths) {
        int w = 0;
        int h = 0;
        int ch = 0;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
        if (!data) {
            std::cerr << "[terrain] failed to load texture array layer: " << path << "\n";
            return false;
        }
        if (w != targetSize || h != targetSize) {
            layers.emplace_back(resampleRGBA(data, w, h, targetSize, targetSize));
            stbi_image_free(data);
        } else {
            layers.emplace_back(data, data + static_cast<size_t>(w * h * 4));
            stbi_image_free(data);
        }
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_id);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, targetSize, targetSize, m_layers,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    for (int i = 0; i < m_layers; ++i) {
        const auto& layer = layers[static_cast<size_t>(i)];
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, targetSize, targetSize, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE, layer.data());
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint wrapMode = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrapMode);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    return true;
}

void TextureArray::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_id);
}

} // namespace nuage
