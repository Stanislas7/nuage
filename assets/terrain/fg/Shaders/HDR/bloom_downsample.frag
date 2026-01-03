$FG_GLSL_VERSION

layout(location = 0) out vec3 fragColor;

in vec2 texcoord;

uniform sampler2D tex;
uniform bool is_first_downsample;

// color.glsl
float linear_srgb_to_luminance(vec3 color);
// bloom.glsl
vec3 bloom_downsample_13tap(sampler2D tex, vec2 uv, vec2 texel_size, bool is_first);

void main()
{
    vec2 texel_size = 1.0 / vec2(textureSize(tex, 0));
    fragColor = bloom_downsample_13tap(tex, texcoord, texel_size, is_first_downsample);
}
