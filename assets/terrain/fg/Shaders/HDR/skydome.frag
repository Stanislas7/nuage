$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in vec3 ray_dir;
in vec3 ray_dir_view;

uniform sampler2D sky_view_tex;
uniform bool is_envmap;

// math.glsl
float M_PI();
float safe_asin(float x);
// atmos_spectral.glsl
vec4 get_sun_outerspace_spectral_irradiance();
vec3 linear_srgb_from_spectral_samples(vec4 L);
// exposure.glsl
vec3 apply_exposure(vec3 color);

void main()
{
    vec3 frag_ray_dir = normalize(ray_dir);
    float azimuth = atan(frag_ray_dir.y, frag_ray_dir.x) / M_PI() * 0.5 + 0.5;
    // Undo the non-linear transformation from the sky-view LUT
    float l = safe_asin(frag_ray_dir.z);
    float elev = sqrt(abs(l) / (M_PI() * 0.5)) * sign(l) * 0.5 + 0.5;

    vec4 inscattering = texture(sky_view_tex, vec2(azimuth, elev));
    // When computing the sky texture we assumed an unitary light source.
    // Now multiply by the sun irradiance.
    vec4 sky_radiance = inscattering * get_sun_outerspace_spectral_irradiance();

    vec3 sky_color = linear_srgb_from_spectral_samples(sky_radiance);

    if (is_envmap == false) {
        // Only pre-expose when not rendering to the environment map.
        // We want the non-exposed radiance values for IBL.
        sky_color = apply_exposure(sky_color);
    }

    fragColor = vec4(sky_color, 1.0);
}
