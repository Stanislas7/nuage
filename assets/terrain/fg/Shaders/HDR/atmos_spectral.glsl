$FG_GLSL_VERSION

const mat4x3 M = mat4x3(
    137.672389239975, -8.632904716299537, -1.7181567391931372,
    32.549094028629234, 91.29801417199785, -12.005406444382531,
    -38.91428392614275, 34.31665471469816, 29.89044807197628,
    8.572844237945445, -11.103384660054624, 117.47585277566478
    );

/*
 * Transform 4 spectral radiance samples to a linear sRGB color.
 * https://www.shadertoy.com/view/msXXDS
 * https://github.com/fgarlin/skytracer
 */
vec3 linear_srgb_from_spectral_samples(vec4 L)
{
    return M * L;
}

/*
 * Extraterrestial Solar Irradiance Spectra, units W * m^-2 * nm^-1
 * https://www.nrel.gov/grid/solar-resource/spectra.html
 */
const vec4 sun_outerspace_spectral_irradiance = vec4(
    1.679, 1.828, 1.986, 1.307);

vec4 get_sun_outerspace_spectral_irradiance()
{
    return sun_outerspace_spectral_irradiance;
}

/*
 * This is just the precalculated result of sun_outerspace_spectral_irradiance
 * converted to linear sRGB using linear_srgb_from_spectral_samples().
 */
// const vec3 sun_outerspace_rgb_irradiance = vec3(0.0);

vec3 get_sun_outerspace_rgb_irradiance()
{
    return linear_srgb_from_spectral_samples(sun_outerspace_spectral_irradiance);
}

/*
 * Sun extraterrestial (not attenuated by the atmospheric medium) spectral
 * radiance in W * m^-2 * nm^-1 * sr^-1.
 *
 * If we consider the Sun as having a relatively small solid angle and a
 * relatively uniform radiance across its surface, we can approximate its
 * radiance as:
 *
 *   radiance = irradiance / solid_angle
 *
 * The solid angle subtended by the Sun varies between 6.64e-5 sr and 7.11e-5 sr
 * depending on the time of the year, but we use 6.8e-5 sr as an average value.
 */
const vec4 sun_outerspace_spectral_radiance = vec4(
    24691.176, 26882.353, 29205.882, 19220.588);

vec4 get_sun_outerspace_spectral_radiance()
{
    return sun_outerspace_spectral_radiance;
}

/*
 * This is just the precalculated result of sun_outerspace_spectral_radiance
 * converted to linear sRGB using linear_srgb_from_spectral_samples().
 */
// const vec3 sun_outerspace_rgb_radiance = vec3(0.0);

vec3 get_sun_outerspace_rgb_radiance()
{
    return linear_srgb_from_spectral_samples(sun_outerspace_spectral_radiance);
}
