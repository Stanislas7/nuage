$FG_GLSL_VERSION

/*
 * Transform an sRGB color to linear sRGB.
 */
vec3 eotf_inverse_sRGB(vec3 srgb)
{
    vec3 a = srgb / 12.92;
    vec3 b = pow((srgb + 0.055) / 1.055, vec3(2.4));
    vec3 c = step(vec3(0.04045), srgb);
    return mix(a, b, c);
}

float eotf_inverse_sRGB_component(float srgb)
{
    float a = srgb / 12.92;
    float b = pow((srgb + 0.055) / 1.055, 2.4);
    float c = step(0.04045, srgb);
    return mix(a, b, c);
}

/*
 * Transform a linear sRGB color to sRGB (gamma correction).
 */
vec3 eotf_sRGB(vec3 linear_srgb)
{
    vec3 a = 12.92 * linear_srgb;
    vec3 b = 1.055 * pow(linear_srgb, vec3(1.0 / 2.4)) - 0.055;
    vec3 c = step(vec3(0.0031308), linear_srgb);
    return mix(a, b, c);
}

float eotf_sRGB_component(float linear_srgb)
{
    float a = 12.92 * linear_srgb;
    float b = 1.055 * pow(linear_srgb, 1.0 / 2.4) - 0.055;
    float c = step(0.0031308, linear_srgb);
    return mix(a, b, c);
}

float linear_srgb_to_luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

float srgb_to_luminance(vec3 color)
{
    vec3 linear = eotf_inverse_sRGB(color);
    float lum = linear_srgb_to_luminance(linear);
    return eotf_sRGB_component(lum);
}
