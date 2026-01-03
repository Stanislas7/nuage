$FG_GLSL_VERSION

out vec4 fragColor;

in VS_OUT {
    vec2 texcoord;
    vec4 vertex_color;
} fs_in;

uniform sampler2D tex;

void main()
{
    vec4 texel = texture(tex, fs_in.texcoord);
    // Modulate by the fill color (stored in the vertex color)
    fragColor = texel * fs_in.vertex_color;
}
