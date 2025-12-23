#pragma once

#include "graphics/glad.h"
#include <string>

namespace flightsim {

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool init(const char* vertexSrc, const char* fragmentSrc);
    void use() const;
    GLint getUniformLocation(const char* name) const;
    GLuint getProgram() const { return m_program; }

private:
    GLuint compileShader(GLenum type, const char* src);
    GLuint m_program = 0;
};

}
