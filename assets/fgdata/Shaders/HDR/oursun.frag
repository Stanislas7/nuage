$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in VS_OUT {
    vec2 texcoord;
    vec3 view_vector;
} fs_in;

uniform vec3 fg_SunDirection;

// math.glsl
float sqr(float x);
float saturate(float x);
// atmos_spectral.glsl
vec4 get_sun_outerspace_spectral_irradiance();
// celestial_body.glsl
vec3 celestial_body_eval_color_spectral(vec4 radiance, vec3 V);

/*
 * Limb darkening
 * http://www.physics.hmc.edu/faculty/esin/a101/limbdarkening.pdf
 */
vec4 get_sun_darkening_factor(float center_to_edge)
{
    const vec4 u = vec4(1.0);
    // Coefficients sampled for wavelengths 630, 560, 490, 430 nm
    const vec4 alpha = vec4(0.429, 0.502, 0.575, 0.643);
    float mu = sqrt(1.0 - sqr(center_to_edge));
    return vec4(1.0) - u * (vec4(1.0) - pow(vec4(mu), alpha));
}

void main()
{
    // The Sun is rendered as a quad with a side length equal to the Sun's
    // diameter. We render the Sun disk by discarding all pixels outside the
    // circle inscribed in the quad.
    float center_to_edge = distance(fs_in.texcoord, vec2(0.5)) * 2.0;
    if (center_to_edge > 1.0) {
        discard;
    }

    vec3 V = normalize(-fs_in.view_vector);
    vec4 sun_radiance = get_sun_outerspace_spectral_irradiance() * 500.0;

    // Darkening factor
    sun_radiance *= get_sun_darkening_factor(center_to_edge);

    fragColor = vec4(celestial_body_eval_color_spectral(sun_radiance, V), 1.0);
}
