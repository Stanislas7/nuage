#pragma once

#include "graphics/glad.h"
#include <vector>
#include <cstdint>

namespace nuage {

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    void init(const std::vector<float>& data);
    void initTextured(const std::vector<float>& data);
    void initIndexed(const std::vector<float>& data, const std::vector<std::uint32_t>& indices);
    void initIndexedTextured(const std::vector<float>& data, const std::vector<std::uint32_t>& indices);
    void draw() const;

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    int m_vertexCount = 0;
    int m_indexCount = 0;
    bool m_indexed = false;
};

}
