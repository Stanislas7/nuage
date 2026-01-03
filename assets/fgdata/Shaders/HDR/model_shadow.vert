$FG_GLSL_VERSION

#pragma import_defines(USE_INSTANCING USE_WINGFLEX_DEFORMATION USE_CHUTE_DEFORMATION)

layout(location = 0) in vec4 pos;
#ifdef USE_INSTANCING
layout(location = 6) in vec3 instance_position; // (x, y, z)
layout(location = 7) in vec4 instance_rotation_and_scale; // (heading, pitch, roll, scale)
#endif

uniform mat4 osg_ModelViewProjectionMatrix;

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
#ifdef USE_WINGFLEX_DEFORMATION
    new_pos.xyz = wingflex_apply_deformation(new_pos.xyz);
#endif
#ifdef USE_CHUTE_DEFORMATION
    new_pos.xyz = chute_apply_deformation(new_pos.xyz);
#endif

#ifdef USE_INSTANCING
    vec3 normal = vec3(0.0);
    apply_instance_transforms(new_pos.xyz,
                              normal,
                              instance_position,
                              instance_rotation_and_scale);
#endif

    gl_Position = osg_ModelViewProjectionMatrix * new_pos;
}
