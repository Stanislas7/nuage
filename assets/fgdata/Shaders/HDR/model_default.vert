$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 vertex_color;
layout(location = 3) in vec4 multitexcoord0;

out VS_OUT {
    float flogz;
    vec2 texcoord;
    vec3 vertex_normal;
    vec4 material_color;
} vs_out;

uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;
uniform mat4 fg_TextureMatrix;

// logarithmic_depth.glsl
float logdepth_prepare_vs_depth(float z);

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * pos;
    vs_out.flogz = logdepth_prepare_vs_depth(gl_Position.w);
    vs_out.texcoord = vec2(fg_TextureMatrix * multitexcoord0);
    vs_out.vertex_normal = osg_NormalMatrix * normal;
    vs_out.material_color = vertex_color;
}
