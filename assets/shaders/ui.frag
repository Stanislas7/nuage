#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uColor;
uniform float uAlpha;

void main() {
    float alpha = texture(uTexture, vTexCoord).r * uAlpha;
    FragColor = vec4(uColor, alpha);
}
