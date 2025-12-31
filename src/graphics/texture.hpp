#pragma once

#include "graphics/glad.h"
#include <string>

namespace nuage {

class Texture {
public:
    Texture() = default;
    ~Texture();

    bool loadFromFile(const std::string& path, bool flipY = true, bool repeat = false);
    void bind(GLuint unit = 0) const;
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
};

}
