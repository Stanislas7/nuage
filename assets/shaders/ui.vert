#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 uProjection;
uniform mat4 uModel;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}
