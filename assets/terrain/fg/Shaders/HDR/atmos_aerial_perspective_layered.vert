$FG_GLSL_VERSION

out VS_OUT {
    vec2 texcoord;
} vs_out;

void main()
{
    vec2 pos = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1.0;
    vs_out.texcoord = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
