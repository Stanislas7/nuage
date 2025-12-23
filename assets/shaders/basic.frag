#version 330 core
in vec3 vColor;
out vec4 FragColor;

uniform vec3 uColor = vec3(1.0, 1.0, 1.0);
uniform bool uUseUniformColor = false;

void main() {
    if (uUseUniformColor) {
        FragColor = vec4(uColor, 1.0);
    } else {
        FragColor = vec4(vColor, 1.0);
    }
}
