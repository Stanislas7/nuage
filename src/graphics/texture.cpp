#include "graphics/texture.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "utils/stb_image.h"

namespace nuage {

Texture::~Texture() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
    }
}

bool Texture::loadFromFile(const std::string& path, bool flipY, bool repeat) {
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << "\n";
        return false;
    }

    std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ", " << channels << "ch)\n";

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 3) {
        format = GL_RGB;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint wrapMode = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return true;
}

bool Texture::loadFromData(const unsigned char* data, int width, int height, int channels, bool repeat) {
    if (!data || width <= 0 || height <= 0 || channels <= 0) {
        return false;
    }
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    GLenum format = GL_RED;
    GLint internalFormat = GL_R8;
    if (channels == 3) {
        format = GL_RGB;
        internalFormat = GL_RGB8;
    } else if (channels == 4) {
        format = GL_RGBA;
        internalFormat = GL_RGBA8;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint wrapMode = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    return true;
}

void Texture::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

}
