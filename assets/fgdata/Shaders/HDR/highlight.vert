$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;

out VS_OUT {
    float flogz;
} vs_out;

uniform mat4 osg_ModelViewProjectionMatrix;

// logarithmic_depth.glsl
float logdepth_prepare_vs_depth(float z);

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * pos;
    vs_out.flogz = logdepth_prepare_vs_depth(gl_Position.w);
}
