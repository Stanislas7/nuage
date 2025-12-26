#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;

void main() {
    vec4 texel = texture(uTexture, vTexCoord);
    if (texel.a < 0.05) discard;
    FragColor = texel;
}
