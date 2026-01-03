$FG_GLSL_VERSION

uniform float alpha;
uniform bool red;
uniform float max_distorsion_amount;

// math.glsl
float sqr(float x);
// color.glsl
float srgb_to_luminance(vec3 color);

vec3 desaturate(vec3 color, float saturation)
{
    float lum = srgb_to_luminance(color);
    return mix(vec3(lum), color, saturation);
}

float opacity(vec2 uv, float amount)
{
    float dist_factor = 1.0 + distance(uv, vec2(0.5)) * 0.3;
    return 1.0 - min(amount * dist_factor, 1.0);
}

/*
 * Distort texcoords according to the intensity of the blackout/redout effect.
 */
vec2 redout_distort(vec2 uv)
{
    // Early exit if there is no blackout/redout
    if (alpha < 1e-5) {
        return uv;
    }
    uv = uv - vec2(0.5);
    float k = mix(0.0, -max_distorsion_amount, alpha);
    return uv * (1.0 + k * dot(uv, uv)) + vec2(0.5);
}

/*
 * Apply the blackout/redout effect.
 */
vec3 redout_apply(vec3 color, vec2 uv)
{
    // Early exit if there is no blackout/redout
    if (alpha < 1e-5) {
        return color;
    }
    color *= opacity(uv, alpha);
    if (red) {
        color = mix(color, vec3(color.r, 0.0, 0.0), alpha);
    } else {
        color = desaturate(color, 1.0 - alpha);
    }
    return color;
}
