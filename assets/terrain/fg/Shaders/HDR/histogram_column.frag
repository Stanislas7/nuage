$FG_GLSL_VERSION

layout(location = 0) out uint fragHits;

uniform usampler2D bins_tex;

void main()
{
    ivec2 bins_tex_size = textureSize(bins_tex, 0);
    int column = int(gl_FragCoord.x);
    uint target_bin = uint(gl_FragCoord.y); // [0, 255]

    uint hits = 0u;

    for (int row = 0; row < bins_tex_size.y; ++row) {
        uint pixel_bin = texelFetch(bins_tex, ivec2(column, row), 0).r;
        // Check if this pixel should go in the bin
        if (pixel_bin == target_bin) {
            hits += 1u;
        }
    }

    fragHits = hits;
}
