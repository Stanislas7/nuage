$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in VS_OUT {
    vec2 texcoord;
    vec3 vertex_normal;
    vec3 view_vector;
} fs_in;

uniform sampler2D color_tex;
uniform sampler2D normal_tex;

FG_VIEW_GLOBAL
uniform vec3 fg_SunDirection[FG_NUM_VIEWS];

// math.glsl
float M_1_4PI();
vec3 remap(vec3 x, float min1, float max1, float min2, float max2);
// normalmap.glsl
vec3 perturb_normal(vec3 N, vec3 V, vec2 texcoord, sampler2D tex);
// atmos_spectral.glsl
vec3 linear_srgb_from_spectral_samples(vec4 L);
vec4 get_sun_outerspace_spectral_irradiance();
// celestial_body.glsl
vec4 celestial_body_transmittance(vec3 V);
// exposure.glsl
vec3 apply_exposure(vec3 color);

void main()
{
    vec3 texel = texture(color_tex, fs_in.texcoord).rgb;
    // The source texture had a gamma of 2.8
    texel = pow(texel, vec3(2.8));
    // Restore original contrast
    texel = remap(texel, 0.0, 1.0, 0.16, 0.4);
    // Normalize by the average intensity of the color map to prevent double
    // dipping the albedo both in the BRDF and in the color map.
    // "Physically Based Real-Time Rendering of the Moon"
    //   Alexander Kuzminykh (2021).
    texel *= 29.873753518; // 1 / 0.0334742

    vec3 N = normalize(fs_in.vertex_normal);
    N = perturb_normal(N, fs_in.view_vector, fs_in.texcoord, normal_tex);

    vec3 L = fg_SunDirection[FG_VIEW_ID];
    vec3 V = normalize(-fs_in.view_vector);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-3);

    // BRDF based on Lommel-Seeliger law. The single scattering albedo w0 has
    // been fitted to visually match the more accurate (and much more expensive)
    // Hapke-Lommel-Seeliger model.
    const float w0 = 0.66;
    float fr = w0 * M_1_4PI() * (1.0 / (NdotL + NdotV));

    // Assume that the Sun irradiance hitting the Moon is the same as the one
    // hitting the Earth.
    vec4 radiance = fr * NdotL * get_sun_outerspace_spectral_irradiance();

    // Apply aerial perspective (atmospheric transmittance)
    radiance *= celestial_body_transmittance(V);
    // Convert from spectral samples to linear sRGB
    vec3 color = linear_srgb_from_spectral_samples(radiance);
    // Add the contribution from the color map
    color *= texel;
    // Pre-expose
    color = apply_exposure(color);

    fragColor = vec4(color, 1.0);
}
