$FG_GLSL_VERSION
/*
 * Render the aerial perspective LUT, similar to
 * "A Scalable and Production Ready Sky and Atmosphere Rendering Technique"
 *     by Sébastien Hillaire (2020).
 *
 * This is the layered rendering version.
 */

layout(triangles, invocations = 32) in;
layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec2 texcoord;
} gs_in[];

out GEOM_OUT {
    vec2 texcoord;
    flat int layer;
} gs_out;

void main()
{
    for (int i = 0; i < gl_in.length(); ++i) {
        gl_Position = gl_in[i].gl_Position;
        gs_out.texcoord = gs_in[i].texcoord;
        gs_out.layer = gl_InvocationID;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}
