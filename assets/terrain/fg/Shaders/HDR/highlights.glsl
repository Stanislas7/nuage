$FG_GLSL_VERSION

uniform sampler2D highlight_tex;

const vec3 HIGHLIGHT_COLOR = vec3(1.0, 0.85, 0.0);
const float HIGHLIGHT_ALPHA = 0.2;

float edge_detection(float x, vec2 uv)
{
    vec2 texel_size = 1.0 / vec2(textureSize(highlight_tex, 0));
    float sum = 0.0;
    sum += textureOffset(highlight_tex, uv, ivec2(-1, -1)).r;
    sum += textureOffset(highlight_tex, uv, ivec2( 0, -1)).r;
    sum += textureOffset(highlight_tex, uv, ivec2( 1, -1)).r;

    sum += textureOffset(highlight_tex, uv, ivec2(-1,  0)).r;
    sum += textureOffset(highlight_tex, uv, ivec2( 1,  0)).r;

    sum += textureOffset(highlight_tex, uv, ivec2(-1,  1)).r;
    sum += textureOffset(highlight_tex, uv, ivec2( 0,  1)).r;
    sum += textureOffset(highlight_tex, uv, ivec2( 1,  1)).r;
    return min(1.0, sum) - x;
}

vec3 highlight_apply(vec3 color, vec2 uv)
{
    float x = texture(highlight_tex, uv).r;
    float edge = edge_detection(x, uv);
    return mix(color, HIGHLIGHT_COLOR, min(1.0, x * HIGHLIGHT_ALPHA + edge));
}
