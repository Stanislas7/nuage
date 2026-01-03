$FG_GLSL_VERSION

FG_VIEW_GLOBAL
uniform mat4 fg_ViewMatrixInverse[FG_NUM_VIEWS];
uniform vec2 fg_FOVCenter[FG_NUM_VIEWS];
uniform vec2 fg_FOVScale[FG_NUM_VIEWS];

// logarithmic_depth.glsl
float logdepth_decode(float z);

/*
 * Reconstruct the view space position from the depth buffer. Mostly used by
 * fullscreen post-processing shaders.
 *
 * Given a 2D screen UV in the range [0,1] and a depth value from the depth
 * buffer, also in the [0,1] range, return the view space position.
 */
vec3 get_view_space_from_depth(vec2 uv, float depth)
{
    float vs_depth = logdepth_decode(depth);
    vec2 half_ndc_pos = fg_FOVCenter[FG_VIEW_ID] - uv;
    vec3 vs_pos = vec3(half_ndc_pos * fg_FOVScale[FG_VIEW_ID] * (-vs_depth), -vs_depth);
    return vs_pos;
}

vec3 get_world_space_from_depth(vec2 uv, float depth)
{
    vec4 vs_p = vec4(get_view_space_from_depth(uv, depth), 1.0);
    return (fg_ViewMatrixInverse[FG_VIEW_ID] * vs_p).xyz;
}
