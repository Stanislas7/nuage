$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;
layout(location = 3) in vec4 multitexcoord0;
layout(location = 4) in vec4 multitexcoord1;

out VS_OUT {
    vec2 p2d_ws;
    vec2 texcoord;
} vs_out;

uniform vec3 fg_modelOffset;

void main()
{
    gl_Position = pos;
    vs_out.p2d_ws = multitexcoord1.xy;
    vs_out.texcoord = multitexcoord0.xy;
}
