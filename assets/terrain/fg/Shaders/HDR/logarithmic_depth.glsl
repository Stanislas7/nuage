$FG_GLSL_VERSION
/*
 * Logarithmic depth buffer utility functions
 *
 * The HDR pipeline uses a logarithmic depth buffer to avoid Z-fighting and
 * depth precision issues. This method is compatible with all OpenGL 3.3
 * hardware since it does not require any fancy depth reversal tricks.
 *
 * The main disadvantage is that we lose the early depth test optimization. This
 * is not a problem for us because we use deferred rendering, so the fragment
 * shaders that run on the actual geometry are very lightweight.
 *
 * Sources:
 * https://outerra.blogspot.com/2009/08/logarithmic-z-buffer.html
 * https://outerra.blogspot.com/2012/11/maximizing-depth-buffer-range-and.html
 * https://outerra.blogspot.com/2013/07/logarithmic-depth-buffer-optimizations.html
 * https://io7m.github.io/r2/documentation/p2s24.xhtml
 */

uniform float fg_Fcoef;
uniform vec2 fg_NearFar;

/*
 * Prepare a view space depth value for encoding. Normal usage involves calling
 * this function in the vertex shader, after writing to gl_Position. The z value
 * corresponds to gl_Position.w (if using perspective projection), or the
 * negated view space depth.
 * See logdepth_encode().
 */
float logdepth_prepare_vs_depth(float z)
{
    return 1.0 + z;
}

/*
 * Encodes a given depth value obtained with logdepth_prepare_vs_depth() into
 * logarithmic depth. The result is used in the fragment shader to write to
 * gl_FragDepth.
 */
float logdepth_encode(float z)
{
    float half_fcoef = fg_Fcoef * 0.5;
    float clamped_z = max(1e-6, z);
    return log2(clamped_z) * half_fcoef;
}

/*
 * This is an alternative way of encoding a depth value obtained with
 * logdepth_prepare_vs_depth() into logarithmic depth in the vertex shader
 * without writing to gl_FragDepth in the fragment shader. The result
 * corresponds to gl_Position.z
 *
 * The disadvantage is that depth will not be interpolated in a
 * perspectively-correct way, but it will work okay for very finely tessellated
 * geometry.
 */
float logdepth_encode_no_perspective(float z)
{
    float clamped_z = max(1e-6, z);
    return log2(clamped_z) * fg_Fcoef - 1.0;
}

/*
 * Decode a view space depth value that was encoded with the functions above.
 * z corresponds to the [0,1] depth buffer value.
 * NOTE: The resulting depth value is positive, so it must be negated to yield
 *       a conventional negative view space depth value.
 */
float logdepth_decode(float z)
{
    float half_fcoef = fg_Fcoef * 0.5;
    float exponent = z / half_fcoef;
    return pow(2.0, exponent) - 1.0;
}

/*
 * Same as logdepth_decode(), but returns the normalized view-space depth
 * in the [0,1] range.
 */
float logdepth_decode_normalized(float z)
{
    return logdepth_decode(z) / fg_NearFar.y;
}
