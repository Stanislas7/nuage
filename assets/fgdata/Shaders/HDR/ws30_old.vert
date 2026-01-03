$FG_GLSL_VERSION

layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec4 multitexcoord0;

out VS_OUT {
    float flogz;
    vec2 texcoord;
    vec2 ground_texcoord;
    vec3 vertex_normal;
    vec3 vs_pos;
    vec3 ws_pos;
    vec2 raw_pos;
    vec3 rel_pos;
    float steepness;
    vec2 grad_dir;
} vs_out;

// From VPBTechnique.cxx
uniform vec3 fg_modelOffset;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;

// logarithmic_depth.glsl
float logdepth_prepare_vs_depth(float z);

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * pos;
    vs_out.flogz = logdepth_prepare_vs_depth(gl_Position.w);
    vs_out.texcoord = multitexcoord0.st;

    // View space normal
    vs_out.vertex_normal = osg_NormalMatrix * normal;

    ///////////////////////////////////////////
    // Test phase code:
    //
    // Coords for ground textures
    // Due to precision issues coordinates should restart (i.e. go to zero) every 5000m or so.
    const float restart_dist_m = 5000.0;
    // Model position
    // vec3 mp = pos.xyz;
    // Temporary approximation to get shaders to compile:
    vs_out.ground_texcoord = vs_out.texcoord;
    // End test phase code
    ///////////////////////////////////////////

    // View space position
    vs_out.vs_pos = vec3(osg_ModelViewMatrix * pos);

    // WS2: worldPos = (osg_ViewMatrixInverse * osg_ModelViewMatrix * pos).xyz;
    vs_out.ws_pos = fg_modelOffset + pos.xyz;

    vec4 vertexZUp = pos;
    // WS2: vs_out.raw_pos = pos.xy;
    vs_out.raw_pos = vertexZUp.xy;

    // XXX: This should be passed as an uniform for performance reasons
    // Transform for frame of reference where:
    //   +z is in the up direction.
    //   The orientation of x and y axes are unknown currently.
    //   The origin is at the same position as the model space origin.
    //   The units are in meters.
    mat4 viewSpaceToZUpSpace = inverse(osg_ModelViewMatrix);
    // Eye position in z up space
    vec4 epZUp = viewSpaceToZUpSpace * vec4(0.0, 0.0, 0.0, 1.0);
    // Position of vertex relative to the eye position in z up space
    vec3 relPosZUp = (vertexZUp - epZUp).xyz;
    vs_out.rel_pos = relPosZUp;

    //vs_out.steepness = abs(dot(normalize(vec3(vec4(normal,1.0))), vec3(0.0, 0.0, 1.0)));
    vs_out.steepness = abs(dot(normalize(normal), vec3(0.0, 0.0, 1.0)));
    // Gradient direction used for small scale noise.
    // In the same space as noise coords, rawpos.xy.
    vs_out.grad_dir = normalize(normal.xy);
}
