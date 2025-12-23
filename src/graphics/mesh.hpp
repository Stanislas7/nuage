#pragma once

#include "graphics/glad.h"
#include <vector>

namespace flightsim {

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    void init(const std::vector<float>& data);
    void draw() const;

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_vertexCount = 0;
};

}
