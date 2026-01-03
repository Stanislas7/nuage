$FG_GLSL_VERSION

layout(location = 0) out vec4 fragColor;

in VS_OUT {
    vec3 view_vector;
    vec4 spectral_irradiance;
} fs_in;

FG_VIEW_GLOBAL
uniform vec4 fg_Viewport[FG_NUM_VIEWS];
uniform mat4 osg_ProjectionMatrix;

// math.glsl
float sqr(float x);
float cub(float x);
// celestial_body.glsl
vec3 celestial_body_eval_color_spectral(vec4 radiance, vec3 V);

/*
 * Returns the solid angle subtended by a pixel.
 *
 * 'p' is the pixel's coordinates, where (0, 0) corresponds to the lower left
 * corner of the screen and 's' corresponds to the top right corner.
 * 'f' is the focal length.
 * All parameters are in pixels, including f.
 *
 * This is an approximation of the exact formula found in [vixra:2001.0603]
 * https://math.stackexchange.com/a/2700457
 */
float solid_angle_pixel_aprox(vec2 p, vec2 s, float f)
{
    float inv_f = 1.0 / max(f, 1e-3);
    float A = sqr(inv_f);
    vec3 X = vec3((p - s * 0.5) * inv_f, 1.0);
    return A / cub(length(X));
}

void main()
{
    vec3 V = normalize(-fs_in.view_vector);

    // Since the star is always a single pixel (they are rendered as GL_POINTS),
    // the solid angle it subtends varies with FOV and its screen position.

    // Focal length is the first element of the projection matrix multiplied by
    // half the image plane width.
    float f = osg_ProjectionMatrix[0][0] * fg_Viewport[FG_VIEW_ID].z * 0.5;
    float omega = solid_angle_pixel_aprox(gl_FragCoord.xy, fg_Viewport[FG_VIEW_ID].zw, f);

    vec4 spectral_radiance = fs_in.spectral_irradiance / max(omega, 1e-8);

    fragColor = vec4(celestial_body_eval_color_spectral(spectral_radiance, V), 1.0);
}
