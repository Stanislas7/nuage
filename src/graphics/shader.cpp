#include "graphics/shader.hpp"
#include "math/mat4.hpp"
#include "math/vec2.hpp"
#include "math/vec3.hpp"
#include <iostream>

namespace nuage {

Shader::~Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
}

bool Shader::init(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(m_program, 512, nullptr, log);
        std::cerr << "Shader link error: " << log << std::endl;
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
}

void Shader::use() const {
    glUseProgram(m_program);
}

GLint Shader::getUniformLocation(const char* name) const {
    return glGetUniformLocation(m_program, name);
}

void Shader::setMat4(const char* name, const Mat4& mat) const {
    GLint loc = getUniformLocation(name);
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, mat.data());
    }
}

void Shader::setVec2(const char* name, const Vec2& vec) const {
    GLint loc = getUniformLocation(name);
    if (loc >= 0) {
        glUniform2f(loc, vec.x, vec.y);
    }
}

void Shader::setVec3(const char* name, const Vec3& vec) const {
    GLint loc = getUniformLocation(name);
    if (loc >= 0) {
        glUniform3f(loc, vec.x, vec.y, vec.z);
    }
}

void Shader::setFloat(const char* name, float value) const {
    GLint loc = getUniformLocation(name);
    if (loc >= 0) {
        glUniform1f(loc, value);
    }
}

void Shader::setBool(const char* name, bool value) const {
    GLint loc = getUniformLocation(name);
    if (loc >= 0) {
        glUniform1i(loc, (int)value);
    }
}

void Shader::setInt(const char* name, int value) const {
    GLint loc = getUniformLocation(name);
    if (loc >= 0) {
        glUniform1i(loc, value);
    }
}

GLuint Shader::compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
    }
    return shader;
}

}
