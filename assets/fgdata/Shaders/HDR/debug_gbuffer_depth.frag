$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in vec2 texcoord;

uniform sampler2D depth_tex;

void main()
{
    float depth = texture(depth_tex, texcoord).r;
    // Display depth as is, no linearization
    fragColor = vec4(vec3(depth), 1.0);
}
