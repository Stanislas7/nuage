$FG_GLSL_VERSION

#pragma import_defines(QUAD_TEXCOORD_RAW)

layout(location = 0) in vec4 pos;
layout(location = 3) in vec4 multitexcoord0;

#ifdef QUAD_TEXCOORD_RAW
out vec2 raw_texcoord;
#endif
out vec2 texcoord;

uniform mat4 osg_ModelViewProjectionMatrix;

// mvr.vert
vec2 mvr_raw_texcoord_transform_fb(vec2 raw_texcoord);

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * pos;
#ifdef QUAD_TEXCOORD_RAW
    raw_texcoord = multitexcoord0.st;
#endif
    texcoord = mvr_raw_texcoord_transform_fb(multitexcoord0.st);
}
