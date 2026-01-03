$FG_GLSL_VERSION
/*
 * The multiple scattering approximation is an implementation of
 * "A Scalable and Production Ready Sky and Atmosphere Rendering Technique"
 *     by Sébastien Hillaire (2020).
 */

layout(location = 0) out vec4 fragColor;

in vec2 texcoord;

uniform sampler2D transmittance_lut;

const int sqrt_samples = 8;
const float inv_sqrt_samples = 1.0 / float(sqrt_samples);
const float inv_samples = inv_sqrt_samples*inv_sqrt_samples;
const int multiple_scattering_steps = 20;

// math.glsl
float M_2PI();
float sqr(float x);
// atmos.glsl
float get_earth_radius();
float get_atmosphere_radius();
vec4 compute_inscattering(in vec3 ray_origin,
                          in vec3 ray_dir,
                          in float t_max,
                          in vec3 sun_dir,
                          in int steps,
                          in sampler2D transmittance_lut,
                          out vec4 f_ms,
                          out vec4 transmittance);

vec3 sample_uniform_sphere(vec2 s)
{
    float phi = M_2PI() * s.x;
    float cos_theta = s.y * 2.0 - 1.0;
    float sin_theta = sqrt(1.0 - sqr(cos_theta));
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

void main()
{
    float sun_cos_theta = texcoord.x * 2.0 - 1.0;
    vec3 sun_dir = vec3(-sqrt(1.0 - sqr(sun_cos_theta)), 0.0, sun_cos_theta);

    float distance_to_earth_center = mix(get_earth_radius(),
                                         get_atmosphere_radius(),
                                         texcoord.y);
    vec3 ray_origin = vec3(0.0, 0.0, distance_to_earth_center);

    vec4 L_2 = vec4(0.0);
    vec4 f_ms = vec4(0.0);

    // Obtain f_ms and the second order scattering (L_2) by integrating over the
    // unit sphere. We use 64 (8x8) uniformly distributed directions.
    for (int i = 0; i < sqrt_samples; ++i) {
        for (int j = 0; j < sqrt_samples; ++j) {
            vec3 ray_dir = sample_uniform_sphere(
                (vec2(i, j) + 0.5) * inv_sqrt_samples);

            vec4 f_ms_partial, transmittance;
            vec4 L_2_partial = compute_inscattering(ray_origin,
                                                    ray_dir,
                                                    1e9,
                                                    sun_dir,
                                                    multiple_scattering_steps,
                                                    transmittance_lut,
                                                    f_ms_partial,
                                                    transmittance);

            // The partials should be multiplied by the sphere solid angle 4pi,
            // but they cancel out with the isotropic phase function.
            L_2 += L_2_partial; // multiplied by 4pi
            f_ms += f_ms_partial; // multiplied by 4pi
        }
    }

    L_2 *= inv_samples; // divided by 4pi
    f_ms *= inv_samples; // divided by 4pi

    // The key point of the paper: assume that the total multiple scattering
    // contribution is a geometric series infinite sum.
    vec4 F_ms = 1.0 / (1.0 - f_ms);

    fragColor = L_2 * F_ms;
}
