$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in vec2 raw_texcoord; // QUAD_TEXCOORD_RAW
in vec2 texcoord;

uniform usampler2D histogram_tex;
uniform bool is_linear;
uniform bool enable_color;

FG_VIEW_GLOBAL
uniform vec4 fg_Viewport[FG_NUM_VIEWS];

// histogram.glsl
float bin_index_to_luminance(float bin, float adapted_luminance);
uint histogram_get_total_pixels(usampler2D histogram_tex);
uint histogram_get_dark_count(uint total_pixels);
uint histogram_get_light_count(uint total_pixels);
bool histogram_nth_bin_passes_dark_check(uint count_after_nth_bin, uint dark_count);
bool histogram_nth_bin_passes_light_check(uint count_before_nth_bin, uint light_count);

void main()
{
    int num_bins = textureSize(histogram_tex, 0).x; // [0, 255]

    int current_index = int(raw_texcoord.x * float(num_bins));
    uint hits = texelFetch(histogram_tex, ivec2(current_index, 0), 0).r;

    vec3 color = vec3(1.0, 1.0, 1.0);
    // Color the histogram bar based on whether the corresponding bin has passed
    // the pixel darkness/brightness check.
    if (enable_color) {
        if (current_index == 0) {
            // The 0th bin is colored in magenta because it's special
            color = vec3(1.0, 0.0, 1.0);
        } else {
            // Get the number of pixels in all bins before this one
            uint prev_pixel_count = 0u;
            for (int i = 1 /* Skip the first bin */; i < current_index; ++i) {
                prev_pixel_count += texelFetch(histogram_tex, ivec2(i, 0), 0).r;
            }

            uint total_pixels = histogram_get_total_pixels(histogram_tex);
            uint dark_count = histogram_get_dark_count(total_pixels);
            uint light_count = histogram_get_light_count(total_pixels);

            bool pass_light = histogram_nth_bin_passes_light_check(
                prev_pixel_count, light_count);
            bool pass_dark = histogram_nth_bin_passes_dark_check(
                prev_pixel_count + hits, dark_count);

            if (!pass_dark) {
                // Pixels that are too dark are blue
                color = vec3(0.0, 0.0, 1.0);
            } else if (!pass_light) {
                // Pixels that are too bright are red
                color = vec3(1.0, 0.0, 0.0);
            } else {
                // Pixels that have been considered valid for the luminance
                // calculation are white.
                color = vec3(1.0, 1.0, 1.0);
            }
        }
    }

    float value = float(hits);
    float max_value = fg_Viewport[FG_VIEW_ID].z * fg_Viewport[FG_VIEW_ID].w;
    if (!is_linear) {
        value = log2(value);
        max_value = log2(max_value);
    }
    float bar = step(texcoord.y * max_value, value);

    fragColor = vec4(color * bar, 1.0);
}
