$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in vec2 texcoord;

uniform sampler2D tex;

void main()
{
    fragColor = texture(tex, texcoord);
}
