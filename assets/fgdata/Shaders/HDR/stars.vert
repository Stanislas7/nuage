$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 spectral_irradiance;

out VS_OUT {
    vec3 view_vector;
    vec4 spectral_irradiance;
} vs_out;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * pos;
    vs_out.view_vector = (osg_ModelViewMatrix * pos).xyz;
    vs_out.spectral_irradiance = spectral_irradiance;
}
