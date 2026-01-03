$FG_GLSL_VERSION
/*
 * Physically-based bloom
 * "Next Generation Post Processing in Call of Duty Advanced Warfare"
 *      ACM Siggraph (2014)
 */

// color.glsl
float linear_srgb_to_luminance(vec3 color);

float karis_average(vec3 color)
{
    return 1.0 / (1.0 + linear_srgb_to_luminance(color));
}

/*
 * Better, temporally stable box filtering
 * Take 13 samples around current texel (g)
 *  a - b - c
 *  - d - e -
 *  f - g - h
 *  - i - j -
 *  k - l - m
 */
vec3 bloom_downsample_13tap(sampler2D tex, vec2 uv, vec2 texel_size, bool is_first)
{
    vec3 a = texture(tex, uv + texel_size * vec2(-1.0,  1.0)).rgb;
    vec3 b = texture(tex, uv + texel_size * vec2( 0.0,  1.0)).rgb;
    vec3 c = texture(tex, uv + texel_size * vec2( 1.0,  1.0)).rgb;
    vec3 d = texture(tex, uv + texel_size * vec2(-0.5,  0.5)).rgb;
    vec3 e = texture(tex, uv + texel_size * vec2( 0.5,  0.5)).rgb;
    vec3 f = texture(tex, uv + texel_size * vec2(-1.0,  0.0)).rgb;
    vec3 g = texture(tex, uv                                ).rgb;
    vec3 h = texture(tex, uv + texel_size * vec2( 1.0, -0.0)).rgb;
    vec3 i = texture(tex, uv + texel_size * vec2(-0.5, -0.5)).rgb;
    vec3 j = texture(tex, uv + texel_size * vec2( 0.5, -0.5)).rgb;
    vec3 k = texture(tex, uv + texel_size * vec2(-1.0, -1.0)).rgb;
    vec3 l = texture(tex, uv + texel_size * vec2( 0.0, -1.0)).rgb;
    vec3 m = texture(tex, uv + texel_size * vec2( 1.0, -1.0)).rgb;

    vec3 downsample;
    if (is_first) {
        // Apply Karis average to each block of 4 samples to prevent fireflies.
        // Only done on the first downsampling step.
        vec3 group0 = (d+e+i+j) * 0.25;
        vec3 group1 = (a+b+f+g) * 0.25;
        vec3 group2 = (b+c+g+h) * 0.25;
        vec3 group3 = (f+g+k+l) * 0.25;
        vec3 group4 = (g+h+l+m) * 0.25;
        float kw0 = karis_average(group0);
        float kw1 = karis_average(group1);
        float kw2 = karis_average(group2);
        float kw3 = karis_average(group3);
        float kw4 = karis_average(group4);
        float kw_sum = kw0 + kw1 + kw2 + kw3 + kw4;
        downsample =
            kw0 * group0 +
            kw1 * group1 +
            kw2 * group2 +
            kw3 * group3 +
            kw4 * group4;
        downsample /= kw_sum;
    } else {
        downsample  = (d+e+i+j) * 0.125;
        downsample += (a+b+f+g) * 0.03125;
        downsample += (b+c+g+h) * 0.03125;
        downsample += (f+g+k+l) * 0.03125;
        downsample += (g+h+l+m) * 0.03125;
    }
    return downsample;
}

/*
 * Takes 9 samples around current texel (e) and applies a 3x3 tent filter
 *  a - b - c
 *  d - e - f
 *  g - h - i
 */
vec3 bloom_upsample_tent(sampler2D tex, vec2 uv, vec2 texel_size)
{
    vec3 a = texture(tex, uv + texel_size * vec2(-1.0,  1.0)).rgb;
    vec3 b = texture(tex, uv + texel_size * vec2( 0.0,  1.0)).rgb;
    vec3 c = texture(tex, uv + texel_size * vec2( 1.0,  1.0)).rgb;

    vec3 d = texture(tex, uv + texel_size * vec2(-1.0,  0.0)).rgb;
    vec3 e = texture(tex, uv                                ).rgb;
    vec3 f = texture(tex, uv + texel_size * vec2( 1.0,  0.0)).rgb;

    vec3 g = texture(tex, uv + texel_size * vec2(-1.0, -1.0)).rgb;
    vec3 h = texture(tex, uv + texel_size * vec2( 0.0, -1.0)).rgb;
    vec3 i = texture(tex, uv + texel_size * vec2( 1.0, -1.0)).rgb;

    vec3 upsample = e * 4.0;
    upsample += (b+d+f+h) * 2.0;
    upsample += (a+c+g+i);
    upsample /= 16.0;

    return upsample;
}

vec3 bloom_apply(vec3 color, vec2 uv, sampler2D tex, float strength)
{
    vec3 bloom = texture(tex, uv).rgb;
    return mix(color, bloom, strength);
}
