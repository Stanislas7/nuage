$FG_GLSL_VERSION

uniform vec4 fg_Viewport[FG_NUM_VIEWS];

const float num_bins = 254.0; // 256 - 2
const float inv_num_bins = 1.0 / num_bins;

// Human eye can see around 10-14 stops
// https://www.cambridgeincolour.com/tutorials/cameras-vs-human-eye.htm
const float log_lum_range = 12.0;

const float min_log_lum_offset = -log_lum_range * 0.5;
const float inv_log_lum_range = 1.0 / log_lum_range;

// Pixels included in the 60th percentile are considered to be too dark and
// pixels outside the 90th percentile are considered too bright.
const float dark_percentile  = 0.60;
const float light_percentile = 0.90;

/*
 * Get the log2 luminance that corresponds to the 0th bin. We do a sliding
 * histogram, i.e. the minimum and maximum luminance in the histogram vary
 * depending on the adapted luminance.
 */
float get_min_log_lum(float adapted_luminance)
{
    return log2(adapted_luminance) + min_log_lum_offset;
}

uint luminance_to_bin_index(float luminance, float adapted_luminance)
{
    float min_log_lum = get_min_log_lum(adapted_luminance);
    // Avoid taking the log of zero
    if (luminance < 1e-6) {
        return 0u;
    }
    // Normalized logarithmic luminance, 0 being the minimum log luminance
    // handled by the histogram, and 1 being the maximum.
    float norm_log_lum = (log2(luminance) - min_log_lum) * inv_log_lum_range;
    norm_log_lum = clamp(norm_log_lum, 0.0, 1.0);
    // From [0, 1] to [1, 255]. The 0th bin is handled by the near-zero check
    return uint(norm_log_lum * num_bins + 1.0);
}

float bin_index_to_luminance(float bin, float adapted_luminance)
{
    float min_log_lum = get_min_log_lum(adapted_luminance);
    return exp2(((bin * inv_num_bins) * log_lum_range) + min_log_lum);
}

/*
 * Get the total amount of valid pixels for the histogram.
 * Basically the screen size minus the pixels in the 0th bin which don't meet
 * the minimum radiance criteria.
 */
uint histogram_get_total_pixels(usampler2D histogram_tex)
{
    uint first_bin_pixels = texelFetch(histogram_tex, ivec2(0), 0).r;
    // histogram represents all views
    uint viewport_pixels = 0;
    for (uint v = 0; v < FG_NUM_VIEWS; ++v)
        viewport_pixels += uint(fg_Viewport[v].z * fg_Viewport[v].w);
    return max(viewport_pixels - first_bin_pixels, 0u);
}

uint histogram_get_dark_count(uint total_pixels)
{
    return uint(dark_percentile * float(total_pixels));
}

uint histogram_get_light_count(uint total_pixels)
{
    return uint(light_percentile * float(total_pixels));
}

bool histogram_nth_bin_passes_dark_check(uint count_after_nth_bin, uint dark_count)
{
    return count_after_nth_bin >= dark_count;
}

bool histogram_nth_bin_passes_light_check(uint count_before_nth_bin, uint light_count)
{
    return count_before_nth_bin <= light_count;
}
