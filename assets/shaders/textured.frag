#version 330 core
in vec3 vNormal;
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uLightDir = vec3(0.0, 1.0, 0.0);
uniform vec3 uLightColor = vec3(1.0, 0.95, 0.9);
uniform vec3 uAmbientColor = vec3(0.25, 0.28, 0.32);

void main() {
    vec3 color = texture(uTexture, vTexCoord).rgb;
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 lighting = uAmbientColor + uLightColor * diffuse;
    FragColor = vec4(color * lighting, 1.0);
}
