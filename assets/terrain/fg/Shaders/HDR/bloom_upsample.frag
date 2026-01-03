$FG_GLSL_VERSION

layout(location = 0) out vec3 fragColor;

in vec2 texcoord;

uniform sampler2D prev_bloom_tex;

// bloom.glsl
vec3 bloom_upsample_tent(sampler2D tex, vec2 uv, vec2 texel_size);

void main()
{
    // The filter does not map to pixels, has "holes" in it. Its radius also
    // varies across mip resolutions.
    vec2 texel_size = 1.0 / textureSize(prev_bloom_tex, 0);
    fragColor = bloom_upsample_tent(prev_bloom_tex, texcoord, texel_size);
}
