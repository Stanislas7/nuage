$FG_GLSL_VERSION

layout(location = 0) out float fragLuminance;

uniform usampler2D histogram_tex;
uniform sampler2D prev_lum_tex;

uniform float osg_DeltaFrameTime;

// Higher values give faster eye adaptation times
const float adapt_speed_dark_to_light = 3.0;
const float adapt_speed_light_to_dark = 1.0;

// histogram.glsl
float bin_index_to_luminance(float bin, float adapted_luminance);
uint histogram_get_total_pixels(usampler2D histogram_tex);
uint histogram_get_dark_count(uint total_pixels);
uint histogram_get_light_count(uint total_pixels);
bool histogram_nth_bin_passes_dark_check(uint count_after_nth_bin, uint dark_count);
bool histogram_nth_bin_passes_light_check(uint count_before_nth_bin, uint light_count);

void main()
{
    uint total_pixels = histogram_get_total_pixels(histogram_tex);
    uint dark_count = histogram_get_dark_count(total_pixels);
    uint light_count = histogram_get_light_count(total_pixels);

    int num_bins = textureSize(histogram_tex, 0).x; // [0, 255]
    uint weighted_sum = 0u;
    uint pixel_count = 0u;
    uint valid_pixel_count = 0u;

    // Calculate the mean of the histogram, skipping parts of it that are too
    // dark or too bright.
    for (int i = 1 /* Skip the first bin */; i < num_bins; ++i) {
        uint hits = texelFetch(histogram_tex, ivec2(i, 0), 0).r;

        // Check if we pass the lightness check before updating the pixel count
        bool pass_light = histogram_nth_bin_passes_light_check(
            pixel_count, light_count);
        // Update the pixel count
        pixel_count += hits;
        // Now the darkness check, after updating
        bool pass_dark = histogram_nth_bin_passes_dark_check(
            pixel_count, dark_count);

        // Only consider the bins that contain pixels that are not too bright
        // or too dark for the mean.
        if (pass_dark && pass_light) {
            weighted_sum += uint(i) * hits;
            valid_pixel_count += hits;
        }
    }

    float mean = float(weighted_sum) / max(float(valid_pixel_count), 1.0);

    // Get the previous adapted luminance
    float prev_lum = max(texelFetch(prev_lum_tex, ivec2(0), 0).r, 1e-6);
    // Transform the bin index [1, 255] to an actual luminance value
    float average_lum = bin_index_to_luminance(mean, prev_lum);

    // Simulate smooth eye adaptation over time
    float lum_diff = average_lum - prev_lum;
    float tau = lum_diff > 0.0 ? adapt_speed_dark_to_light : adapt_speed_light_to_dark;
    float adapted_lum = prev_lum + lum_diff * (1.0 - exp(-osg_DeltaFrameTime * tau));

    fragLuminance = adapted_lum;
}
