$FG_GLSL_VERSION
/*
 * Render the aerial perspective LUT, similar to
 * "A Scalable and Production Ready Sky and Atmosphere Rendering Technique"
 *     by Sébastien Hillaire (2020).
 *
 * This is the layered rendering version.
 */

layout(location = 0) out vec4 fragColor;

in GEOM_OUT {
    vec2 texcoord;
    flat int layer;
} fs_in;

uniform sampler2D transmittance_tex;
uniform sampler2D ms_tex;

FG_VIEW_GLOBAL
uniform mat4 fg_ViewMatrixInverse[FG_NUM_VIEWS];
uniform vec3 fg_CameraPositionCart;
uniform vec3 fg_SunDirectionWorld;

// pos_from_depth.glsl
vec3 get_view_space_from_depth(vec2 uv, float depth);
// atmos.glsl
float get_earth_radius();
vec4 compute_inscattering(in vec3 ray_origin,
                          in vec3 ray_dir,
                          in float t_max,
                          in vec3 sun_dir,
                          in int steps,
                          in sampler2D transmittance_lut,
                          in sampler2D ms_lut,
                          out vec4 transmittance);
// atmos_spectral.glsl
vec4 get_sun_outerspace_spectral_irradiance();
vec3 linear_srgb_from_spectral_samples(vec4 L);
// aerial_perspective.glsl
vec2 ap_get_uv_for_voxel(uvec2 voxel);
float ap_slice_to_depth(float slice);
float ap_apply_squared_distribution(float slice);

void main()
{
    vec3 ray_dir_vs = normalize(get_view_space_from_depth(fs_in.texcoord, 1.0));
    vec3 ray_dir = vec4(fg_ViewMatrixInverse[FG_VIEW_ID] * vec4(ray_dir_vs, 0.0)).xyz;

    vec3 ray_origin = fg_CameraPositionCart;

    float slice = float(fs_in.layer) + 1.0;
    slice = ap_apply_squared_distribution(slice);

    float depth = ap_slice_to_depth(slice);
    // Intersection of the ray with a plane at a given depth
    float t_max = -depth / dot(vec3(0.0, 0.0, 1.0), ray_dir_vs);

    vec3 ray_end = ray_origin + ray_dir * t_max;
    float distance_to_earth_center = length(ray_end);
    if (distance_to_earth_center <= get_earth_radius()) {
        // Apply a position offset to make sure no artifacts are visible close
        // to the earth boundaries for large voxel.
        ray_end = normalize(ray_end) * get_earth_radius();
        ray_dir = normalize(ray_end - ray_origin);
        t_max = length(ray_end - ray_origin);
    }

    // Variable step count depending on the slice depth (far slices require
    // more steps than near slices).
    int steps = int(fs_in.layer / 2 + 1);

    vec4 transmittance;
    vec4 L = compute_inscattering(ray_origin,
                                  ray_dir,
                                  t_max,
                                  fg_SunDirectionWorld,
                                  steps,
                                  transmittance_tex,
                                  ms_tex,
                                  transmittance);

    vec3 L_rgb = linear_srgb_from_spectral_samples(
        L * get_sun_outerspace_spectral_irradiance());
    float average_transmittance = dot(transmittance, vec4(0.25));

    fragColor = vec4(L_rgb, average_transmittance);
}
