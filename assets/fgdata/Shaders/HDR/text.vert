$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec4 multitexcoord0;

out VS_OUT {
    float flogz;
    vec2 texcoord;
    vec3 vertex_normal;
    vec3 view_vector;
    vec4 ap_color;
} vs_out;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;
uniform mat4 fg_TextureMatrix;

// aerial_perspective.glsl
vec4 get_aerial_perspective(vec2 raw_coord, vec3 P);
// logarithmic_depth.glsl
float logdepth_prepare_vs_depth(float z);

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * pos;
    vs_out.flogz = logdepth_prepare_vs_depth(gl_Position.w);
    vs_out.texcoord = vec2(fg_TextureMatrix * multitexcoord0);
    vs_out.vertex_normal = osg_NormalMatrix * normal;
    vs_out.view_vector = (osg_ModelViewMatrix * pos).xyz;

    vec2 raw_coord = (gl_Position.xy / gl_Position.w) * 0.5 + 0.5;
    vs_out.ap_color = get_aerial_perspective(raw_coord, vs_out.view_vector);
}
