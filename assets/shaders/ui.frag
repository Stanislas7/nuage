#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uColor;
uniform float uAlpha;
uniform int uRounded;
uniform vec2 uRectSize;
uniform float uRadius;

void main() {
    float alpha = texture(uTexture, vTexCoord).r * uAlpha;
    if (uRounded == 1) {
        vec2 halfSize = 0.5 * uRectSize;
        vec2 local = vTexCoord * uRectSize - halfSize;
        vec2 d = abs(local) - (halfSize - vec2(uRadius));
        float dist = length(max(d, vec2(0.0))) + min(max(d.x, d.y), 0.0) - uRadius;
        float aa = fwidth(dist);
        float mask = smoothstep(aa, -aa, dist);
        alpha *= mask;
    }
    FragColor = vec4(uColor, alpha);
}
