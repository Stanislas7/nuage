$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;
layout(location = 3) in vec4 multitexcoord0;

out VS_OUT {
    vec2 texcoord;
    vec3 view_vector;
} vs_out;

uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * pos;
    vs_out.texcoord = multitexcoord0.st;
    vs_out.view_vector = (osg_ModelViewMatrix * pos).xyz;
}
