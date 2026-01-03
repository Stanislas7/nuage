$FG_GLSL_VERSION

#pragma import_defines(USE_INSTANCING USE_WINGFLEX_DEFORMATION USE_CHUTE_DEFORMATION)

layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec4 multitexcoord0;
#ifdef USE_INSTANCING
layout(location = 6) in vec3 instance_position; // (x, y, z)
layout(location = 7) in vec4 instance_rotation_and_scale; // (heading, pitch, roll, scale)
#endif

out VS_OUT {
    float flogz;
    vec2 texcoord;
    vec3 vertex_normal;
    vec3 view_vector;
} vs_out;

uniform bool flip_vertically;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;
uniform mat4 fg_TextureMatrix;

// logarithmic_depth.glsl
float logdepth_prepare_vs_depth(float z);

#ifdef USE_INSTANCING
// object_instancing.glsl
void apply_instance_transforms(inout vec3 position,
                               inout vec3 normal,
                               in vec3 instance_position,
                               in vec4 instance_rotation_and_scale);
#endif
#ifdef USE_WINGFLEX_DEFORMATION
// wingflex.glsl
vec3 wingflex_apply_deformation(vec3 pos);
#endif
#ifdef USE_CHUTE_DEFORMATION
// chute.glsl
vec3 chute_apply_deformation(vec3 pos);
#endif

void main()
{
    vec4 new_pos = pos;
    vec3 new_normal = normal;
#ifdef USE_WINGFLEX_DEFORMATION
    new_pos.xyz = wingflex_apply_deformation(new_pos.xyz);
#endif
#ifdef USE_CHUTE_DEFORMATION
    new_pos.xyz = chute_apply_deformation(new_pos.xyz);
#endif

#ifdef USE_INSTANCING
    apply_instance_transforms(new_pos.xyz,
                              new_normal,
                              instance_position,
                              instance_rotation_and_scale);
#endif

    gl_Position = osg_ModelViewProjectionMatrix * new_pos;
    vs_out.flogz = logdepth_prepare_vs_depth(gl_Position.w);
    vs_out.texcoord = vec2(fg_TextureMatrix * multitexcoord0);
    if (flip_vertically)
        vs_out.texcoord.y = 1.0 - vs_out.texcoord.y;

    vs_out.vertex_normal = osg_NormalMatrix * new_normal;
    vs_out.view_vector = (osg_ModelViewMatrix * new_pos).xyz;
}
