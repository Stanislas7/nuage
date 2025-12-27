#pragma once

#include "graphics/glad.h"
#include <string>

namespace nuage {

struct Mat4;
struct Vec2;
struct Vec3;

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool init(const char* vertexSrc, const char* fragmentSrc);
    void use() const;
    GLint getUniformLocation(const char* name) const;
    GLuint getProgram() const { return m_program; }
    
    void setMat4(const char* name, const Mat4& mat) const;
    void setVec2(const char* name, const Vec2& vec) const;
    void setVec3(const char* name, const Vec3& vec) const;
    void setFloat(const char* name, float value) const;
    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;

private:
    GLuint compileShader(GLenum type, const char* src);
    GLuint m_program = 0;
};

}
