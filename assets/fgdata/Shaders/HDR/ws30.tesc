$FG_GLSL_VERSION

layout (vertices = 4) out;

in VS_OUT {
    vec2 p2d_ws;
    vec2 texcoord;
} tcs_in[];

out TCS_OUT {
    vec2 p2d_ls;
    vec2 p2d_ws;
    vec2 texcoord;
} tcs_out[];

patch out mat4 patch_heights;
patch out vec2 patch_size_inv;

uniform mat4 osg_ModelViewMatrix;

// Maximum and minimum tessellation levels
const float min_tess_level = 1.0;
const float max_tess_level = 33.0;
// Distance to camera for maximum tessellation
const float log_min_distance = log2(40.0);
// Distance to camera for minimum tessellation
const float log_max_distance = log2(40000.0);
const float inv_log_distance_range = 1.0 / (log_max_distance - log_min_distance);

/*
 * Return a normalized [0,1] distance to the camera of a vertex in view space.
 * 0 corresponds to the closest possible distance, and 1 to the furthest.
 *
 * This distance is not linear, i.e. 0.5 is not half the distance betweeen the
 * vertex and the camera.
 */
float normalized_distance_to_eye(vec3 p_vs)
{
    float log_d = log2(length(p_vs));
    float norm = clamp((log_d - log_min_distance) * inv_log_distance_range, 0.0, 1.0);
    float x = norm - 1.0;
    return 1.0 - x*x;
}

/*
 * Calculate the edge tessellation level based on the distance to the camera of
 * the closer vertex.
 */
float edge_tess_level_from_distance_to_eye(float d0, float d1)
{
    return mix(max_tess_level, min_tess_level, min(d0, d1));
}

void main()
{
    switch (gl_InvocationID) {
    case 0: // p00
        tcs_out[gl_InvocationID].p2d_ls = gl_in[5].gl_Position.xy;
        tcs_out[gl_InvocationID].p2d_ws = tcs_in[5].p2d_ws;
        tcs_out[gl_InvocationID].texcoord = tcs_in[5].texcoord;
        break;
    case 1: // p01
        tcs_out[gl_InvocationID].p2d_ls = gl_in[9].gl_Position.xy;
        tcs_out[gl_InvocationID].p2d_ws = tcs_in[9].p2d_ws;
        tcs_out[gl_InvocationID].texcoord = tcs_in[9].texcoord;
        break;
    case 2: // p10
        tcs_out[gl_InvocationID].p2d_ls = gl_in[6].gl_Position.xy;
        tcs_out[gl_InvocationID].p2d_ws = tcs_in[6].p2d_ws;
        tcs_out[gl_InvocationID].texcoord = tcs_in[6].texcoord;
        break;
    case 3: // p11
        tcs_out[gl_InvocationID].p2d_ls = gl_in[10].gl_Position.xy;
        tcs_out[gl_InvocationID].p2d_ws = tcs_in[10].p2d_ws;
        tcs_out[gl_InvocationID].texcoord = tcs_in[10].texcoord;
        break;
    }

    if (gl_InvocationID == 0) {
        // Send the heights of all 16 neighbouring vertices
        patch_heights = mat4(gl_in[ 0].gl_Position.z,
                             gl_in[ 1].gl_Position.z,
                             gl_in[ 2].gl_Position.z,
                             gl_in[ 3].gl_Position.z,
                             gl_in[ 4].gl_Position.z,
                             gl_in[ 5].gl_Position.z,
                             gl_in[ 6].gl_Position.z,
                             gl_in[ 7].gl_Position.z,
                             gl_in[ 8].gl_Position.z,
                             gl_in[ 9].gl_Position.z,
                             gl_in[10].gl_Position.z,
                             gl_in[11].gl_Position.z,
                             gl_in[12].gl_Position.z,
                             gl_in[13].gl_Position.z,
                             gl_in[14].gl_Position.z,
                             gl_in[15].gl_Position.z);

        patch_size_inv = 1.0 / vec2(distance(gl_in[6].gl_Position.xy, gl_in[5].gl_Position.xy),
                                    distance(gl_in[9].gl_Position.xy, gl_in[5].gl_Position.xy));

        // The tessellation level cannot be simply based on the distance to the
        // patch, or there would be discontinuities in the patch boundaries.
        // Instead, we compute the outer tessellation levels based on the distance
        // of each edge to the camera.

        vec3 p00_vs = vec3(osg_ModelViewMatrix * gl_in[5].gl_Position);
        vec3 p01_vs = vec3(osg_ModelViewMatrix * gl_in[9].gl_Position);
        vec3 p10_vs = vec3(osg_ModelViewMatrix * gl_in[6].gl_Position);
        vec3 p11_vs = vec3(osg_ModelViewMatrix * gl_in[10].gl_Position);

        float d00 = normalized_distance_to_eye(p00_vs);
        float d01 = normalized_distance_to_eye(p01_vs);
        float d10 = normalized_distance_to_eye(p10_vs);
        float d11 = normalized_distance_to_eye(p11_vs);

        float tess_level0 = edge_tess_level_from_distance_to_eye(d01, d00); // left
        float tess_level1 = edge_tess_level_from_distance_to_eye(d00, d10); // bottom
        float tess_level2 = edge_tess_level_from_distance_to_eye(d10, d11); // right
        float tess_level3 = edge_tess_level_from_distance_to_eye(d11, d01); // top

        float inner_tess_level0 = max(tess_level1, tess_level3); // top & bottom
        float inner_tess_level1 = max(tess_level0, tess_level2); // left & right

        gl_TessLevelOuter[0] = tess_level0;
        gl_TessLevelOuter[1] = tess_level1;
        gl_TessLevelOuter[2] = tess_level2;
        gl_TessLevelOuter[3] = tess_level3;

        gl_TessLevelInner[0] = inner_tess_level0;
        gl_TessLevelInner[1] = inner_tess_level1;
    }
}
