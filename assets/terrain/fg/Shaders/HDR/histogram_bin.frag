$FG_GLSL_VERSION

layout(location = 0) out uint fragBin;

uniform sampler2D hdr_tex;
uniform sampler2D lum_tex;

// color.glsl
float linear_srgb_to_luminance(vec3 color);
// histogram.glsl
uint luminance_to_bin_index(float luminance, float adapted_luminance);
// exposure.glsl
vec3 undo_exposure(vec3 color);

void main()
{
    vec3 hdr_color = texelFetch(hdr_tex, ivec2(gl_FragCoord), 0).rgb;
    hdr_color = undo_exposure(hdr_color);
    // sRGB to relative luminance
    float lum = linear_srgb_to_luminance(hdr_color);
    // Get the bin index corresponding to the given pixel luminance
    float adapted_luminance = max(texelFetch(lum_tex, ivec2(0), 0).r, 1e-8);
    fragBin = luminance_to_bin_index(lum, adapted_luminance);
}
