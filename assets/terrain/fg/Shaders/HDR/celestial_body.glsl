$FG_GLSL_VERSION

uniform sampler2D transmittance_tex;

FG_VIEW_GLOBAL
uniform float fg_CameraDistanceToEarthCenter;
uniform float fg_EarthRadius;
uniform vec3 fg_CameraViewUp[FG_NUM_VIEWS];

const float ATMOSPHERE_RADIUS = 6471e3;

// math.glsl
float saturate(float x);
// atmos_spectral.glsl
vec3 linear_srgb_from_spectral_samples(vec4 L);
// exposure.glsl
vec3 apply_exposure(vec3 color);

vec4 celestial_body_transmittance(vec3 V)
{
    float normalized_altitude =
        (fg_CameraDistanceToEarthCenter - fg_EarthRadius)
        / (ATMOSPHERE_RADIUS - fg_EarthRadius);
    float cos_theta = dot(V, fg_CameraViewUp[FG_VIEW_ID]);

    vec2 uv = vec2(saturate(cos_theta * 0.5 + 0.5),
                   saturate(normalized_altitude));
    return texture(transmittance_tex, uv);
}

/*
 * Apply aerial perspective (atmospheric transmittance) and pre-expose a
 * celestial body in the sky.
 * This assumes radiance is an RGB color, i.e. not spectral samples.
 */
vec3 celestial_body_eval_color(vec3 radiance, vec3 V)
{
    // Apply aerial perspective by averaging the transmittance spectral samples.
    // The proper thing would be to have spectral data for the radiance of the
    // celestial object (or albedo in case of a diffuse surface like the Moon's)
    vec4 transmittance = celestial_body_transmittance(V);
    vec3 color = radiance * dot(transmittance, vec4(0.25));

    // Pre-expose
    color = apply_exposure(color);

    // Final color = transmittance * radiance + sky inscattering
    // This function outputs the multiplication part, and the sky in-scattering
    // is added by doing additive blending on top of the skydome.
    return color;
}

/*
 * Same as above but assuming that the given radiance is a spectrum.
 */
vec3 celestial_body_eval_color_spectral(vec4 radiance, vec3 V)
{
    // Apply aerial perspective
    radiance *= celestial_body_transmittance(V);
    // Convert from spectrum to sRGB
    vec3 color = linear_srgb_from_spectral_samples(radiance);

    // Pre-expose
    color = apply_exposure(color);

    // We use a small float texture format, which guarantees a maximum value of
    // 65000. We need to clamp our lighting for very bright objects (mainly the
    // Sun).
    color = min(color, 1e4);

    // Final color = transmittance * radiance + sky inscattering
    // This function outputs the multiplication part, and the sky in-scattering
    // is added by doing additive blending on top of the skydome.
    return color;
}
