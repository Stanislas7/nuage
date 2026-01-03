$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in vec2 raw_texcoord; // QUAD_TEXCOORD_RAW
in vec2 texcoord;

uniform sampler2D hdr_tex;
uniform sampler2D bloom_tex;

uniform vec2 fg_BufferSize;

uniform float bloom_strength;
uniform bool debug_ev100;

// math.glsl
float interleaved_gradient_noise(vec2 uv);
// color.glsl
vec3 eotf_sRGB(vec3 linear_srgb);
// aces.glsl
vec3 aces_fitted(vec3 color);
// bloom.glsl
vec3 bloom_apply(vec3 color, vec2 uv, sampler2D tex, float strength);
// redout.glsl
vec2 redout_distort(vec2 uv);
vec3 redout_apply(vec3 color, vec2 uv);
// night-vision.glsl
vec3 night_vision_apply(vec3 color, vec2 uv);
// highlights.glsl
vec3 highlight_apply(vec3 color, vec2 uv);
// mvr.frag
vec2 mvr_raw_texcoord_transform_fb(vec2 raw_texcoord);

vec3 get_debug_color(float value)
{
    float level = value * 3.14159265 * 0.5;
    vec3 col;
    col.r = sin(level);
    col.g = sin(level * 2.0);
    col.b = cos(level);
    return col;
}

vec3 get_ev100_color(vec3 hdr)
{
    float level;
    if (raw_texcoord.y < 0.05) {
        const float w = 0.001;
        if (raw_texcoord.x >= (0.5 - w) && raw_texcoord.x <= (0.5 + w))
            return vec3(1.0);
        return get_debug_color(raw_texcoord.x);
    }
    float lum = max(dot(hdr, vec3(0.299, 0.587, 0.114)), 0.0001);
    float ev100 = log2(lum * 8.0);
    float norm = ev100 / 12.0 + 0.5;
    return get_debug_color(norm);
}

void main()
{
    if (debug_ev100) {
        fragColor = vec4(get_ev100_color(texture(hdr_tex, texcoord).rgb), 1.0);
        return;
    }

    // Blackout/redout applies a small distortion effect
    vec2 raw_uv = redout_distort(raw_texcoord);
    vec2 uv = mvr_raw_texcoord_transform_fb(raw_uv);

    vec3 hdr_color = texture(hdr_tex, uv).rgb;
    // Apply bloom
    hdr_color = bloom_apply(hdr_color, uv, bloom_tex, bloom_strength);
    // Tonemap
    vec3 color = aces_fitted(hdr_color);
    // Apply night vision filter
    color = night_vision_apply(color, uv);
    // Apply blackout/redout
    color = redout_apply(color, raw_uv);
    // Gamma correction
    color = eotf_sRGB(color);

    // Pick animation highlights
    color = highlight_apply(color, uv);

    // Dithering to reduce banding
    color += (1.0 / 255.0) * interleaved_gradient_noise(gl_FragCoord.xy) - (0.5 / 255.0);

    fragColor = vec4(color, 1.0);
}
