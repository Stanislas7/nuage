$FG_GLSL_VERSION

/*
 * Dither according to a checkerboard pattern.
 *   t < 0.25 -> 0                 [1 0]
 *   t > 0.75 -> 1                 [0 1]
 *   0.25 < t < 0.75 -> 2x2 checkerboard pattern above
 */
float dither_checkerboard(vec2 pixel_coord, float t)
{
    float v = fract(dot(pixel_coord, vec2(0.5)));
    return step(0.75, t) + step(0.25, t) * step(0.5, v);
}
