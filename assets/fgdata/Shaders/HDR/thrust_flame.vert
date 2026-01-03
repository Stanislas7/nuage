$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;

out VS_OUT {
    float flogz;
    vec3 ls_pos;
    vec3 view_dir;
} vs_out;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;

// logarithmic_depth.glsl
float logdepth_prepare_vs_depth(float z);

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * pos;
    vs_out.flogz = logdepth_prepare_vs_depth(gl_Position.w);
    vs_out.ls_pos = pos.xyz;

    // XXX: no osg_ModelViewMatrixInverse
    vec4 ep = inverse(osg_ModelViewMatrix) * vec4(0.0, 0.0, 0.0, 1.0);
    vs_out.view_dir = normalize(pos.xyz - ep.xyz);
}
