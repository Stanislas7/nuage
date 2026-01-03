#pragma once

#include "graphics/glad.h"
#include <string>

namespace nuage {

class Texture {
public:
    Texture() = default;
    ~Texture();

    bool loadFromFile(const std::string& path, bool flipY = true, bool repeat = false);
    bool loadFromData(const unsigned char* data, int width, int height, int channels,
                      bool repeat = false, bool nearest = false, bool generateMipmaps = true);
    void bind(GLuint unit = 0) const;
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
};

}
