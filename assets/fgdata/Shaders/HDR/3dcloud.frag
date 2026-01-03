$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in float flogz;

// 3dcloud_common.frag
vec4 cloud_common_frag();
// exposure.glsl
vec3 apply_exposure(vec3 color);
// logarithmic_depth.glsl
float logdepth_encode(float z);

void main()
{
    vec4 color = cloud_common_frag();

    // Only pre-expose when not rendering to the environment map.
    // We want the non-exposed radiance values for IBL.
    color.rgb = apply_exposure(color.rgb);

    fragColor = color;
    gl_FragDepth = logdepth_encode(flogz);
}
