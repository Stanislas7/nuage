$FG_GLSL_VERSION

#pragma import_defines(QUAD_TEXCOORD_RAW)

#ifdef QUAD_TEXCOORD_RAW
out vec2 raw_texcoord;
#endif
out vec2 texcoord;

// mvr.vert
vec2 mvr_raw_texcoord_transform_fb(vec2 raw_texcoord);

void main()
{
    vec2 pos = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1.0;
    vec2 loc_raw_texcoord = pos * 0.5 + 0.5;
#ifdef QUAD_TEXCOORD_RAW
    raw_texcoord = loc_raw_texcoord;
#endif
    texcoord = mvr_raw_texcoord_transform_fb(loc_raw_texcoord);
    gl_Position = vec4(pos, 0.0, 1.0);
}
