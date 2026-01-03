$FG_GLSL_VERSION

#define NOISE_SMOOTH_INTERP_METHOD 0

// We define pi locally here to avoid including math.glsl
// noise.glsl should be independent.
const float NOISE_PI = 3.14159265358979323846;

// Based on the best integer hash by Chris Wellons and TheIronBorn
// https://www.shadertoy.com/view/dllSW7
// https://github.com/skeeto/hash-prospector/issues/23
uint hash(uint x)
{
    x ^= x >> 15;
    x = (x | 1u) ^ (x * x);
    x ^= x >> 17;
    x *= 0x9E3779B9u;
    x ^= x >> 13;
    return x;
}

// Compound versions
uint hash(uvec2 x) {
    return hash(x.x ^ hash(x.y));
}
uint hash(uvec3 x) {
    return hash(x.x ^ hash(x.y) ^ hash(x.z));
}
uint hash(uvec4 x) {
    return hash(x.x ^ hash(x.y) ^ hash(x.z) ^ hash(x.w));
}

float uint_to_normalized_float(uint x) {
    return uintBitsToFloat(x & 0x007FFFFFu | 0x3F800000u) - 1.0;
}

// Random value using unsigned integer seeds
float rand_1d(uint x) {
    return uint_to_normalized_float(hash(x));
}
float rand_2d(uvec2 x) {
    return uint_to_normalized_float(hash(x));
}
float rand_3d(uvec3 x) {
    return uint_to_normalized_float(hash(x));
}
float rand_4d(uvec4 x) {
    return uint_to_normalized_float(hash(x));
}

// Random value using floating point seeds
float rand_1d(float x) {
    return uint_to_normalized_float(hash(floatBitsToUint(x)));
}
float rand_2d(vec2 x) {
    return uint_to_normalized_float(hash(floatBitsToUint(x)));
}
float rand_3d(vec3 x) {
    return uint_to_normalized_float(hash(floatBitsToUint(x)));
}
float rand_4d(vec4 x) {
    return uint_to_normalized_float(hash(floatBitsToUint(x)));
}


vec2 smooth_interp(vec2 a)
{
#if NOISE_SMOOTH_INTERP_METHOD == 0 // Cubic Hermite
    return smoothstep(0.0, 1.0, a);
#elif NOISE_SMOOTH_INTERP_METHOD == 1 // Quintic Hermite
    return a * a * a * (a * (a * 6.0 - 15.0) + 10.0);
#elif NOISE_SMOOTH_INTERP_METHOD == 2 // Cosine
    return (1.0 - cos(a * NOISE_PI)) * 0.5;
#else // Linear
    return a;
#endif
}

vec3 smooth_interp(vec3 a)
{
#if NOISE_SMOOTH_INTERP_METHOD == 0 // Cubic Hermite
    return smoothstep(0.0, 1.0, a);
#elif NOISE_SMOOTH_INTERP_METHOD == 1 // Quintic Hermite
    return a * a * a * (a * (a * 6.0 - 15.0) + 10.0);
#elif NOISE_SMOOTH_INTERP_METHOD == 2 // Cosine
    return (1.0 - cos(a * NOISE_PI)) * 0.5;
#else // Linear
    return a;
#endif
}

vec2 smooth_interp_derivative(vec2 a)
{
#if NOISE_SMOOTH_INTERP_METHOD == 0 // Cubic Hermite
    return 6.0 * a * (1.0 - a);
#elif NOISE_SMOOTH_INTERP_METHOD == 1 // Quintic Hermite
    return 30.0 * a * a * (a * (a - 2.0) + 1.0);
#elif NOISE_SMOOTH_INTERP_METHOD == 2 // Cosine
    return (NOISE_PI * sin(a * NOISE_PI)) * 0.5;
#else // Linear
    return a;
#endif
}

vec3 smooth_interp_derivative(vec3 a)
{
#if NOISE_SMOOTH_INTERP_METHOD == 0 // Cubic Hermite
    return 6.0 * a * (1.0 - a);
#elif NOISE_SMOOTH_INTERP_METHOD == 1 // Quintic Hermite
    return 30.0 * a * a * (a * (a - 2.0) + 1.0);
#elif NOISE_SMOOTH_INTERP_METHOD == 2 // Cosine
    return (NOISE_PI * sin(a * NOISE_PI)) * 0.5;
#else // Linear
    return a;
#endif
}

float bilinear_interp(float p00, float p01, float p10, float p11, vec2 u)
{
    vec2 px = mix(vec2(p00, p01), vec2(p10, p11), u.x);
    return mix(px.x, px.y, u.y);
}

float trilinear_interp(float p000, float p001, float p010, float p011,
                       float p100, float p101, float p110, float p111,
                       vec3 u)
{
    vec4 pz = mix(vec4(p000, p010, p100, p110),
                  vec4(p001, p011, p101, p111),
                  u.z);
    vec2 px = mix(pz.xy, pz.zw, u.x);
    return mix(px.x, px.y, u.y);
}

// 2D value noise
float noise_2d(vec2 x)
{
    vec2 i = floor(x);
    vec2 f = fract(x);
    vec2 u = smooth_interp(f);
    float g00 = rand_2d(i + vec2(0.0, 0.0));
    float g10 = rand_2d(i + vec2(1.0, 0.0));
    float g01 = rand_2d(i + vec2(0.0, 1.0));
    float g11 = rand_2d(i + vec2(1.0, 1.0));
    return bilinear_interp(g00, g01, g10, g11, u);
}

// 2D value noise with derivatives
vec3 noised_2d(vec2 x)
{
    vec2 i = floor(x);
    vec2 f = fract(x);
    vec2 u = smooth_interp(f);
    vec2 du = smooth_interp_derivative(f);
    float g00 = rand_2d(i + vec2(0.0, 0.0));
    float g10 = rand_2d(i + vec2(1.0, 0.0));
    float g01 = rand_2d(i + vec2(0.0, 1.0));
    float g11 = rand_2d(i + vec2(1.0, 1.0));
    vec2 gx = mix(vec2(g00, g01), vec2(g10, g11), u.x);
    vec2 gy = mix(vec2(g00, g10), vec2(g01, g11), u.y);
    float dndx = (gy.y - gy.x) * du.x;
    float dndy = (gx.y - gx.x) * du.y;
    float n = mix(gx.x, gx.y, u.y); // Can also be mix(gy.x, gy.y, u.x)
    return vec3(n, dndx, dndy);
}

// 3D value noise
float noise_3d(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    vec3 u = smooth_interp(f);
    float g000 = rand_3d(i + vec3(0.0, 0.0, 0.0));
    float g100 = rand_3d(i + vec3(1.0, 0.0, 0.0));
    float g010 = rand_3d(i + vec3(0.0, 1.0, 0.0));
    float g110 = rand_3d(i + vec3(1.0, 1.0, 0.0));
    float g001 = rand_3d(i + vec3(0.0, 0.0, 1.0));
    float g101 = rand_3d(i + vec3(1.0, 0.0, 1.0));
    float g011 = rand_3d(i + vec3(0.0, 1.0, 1.0));
    float g111 = rand_3d(i + vec3(1.0, 1.0, 1.0));
    return trilinear_interp(g000, g001, g010, g011,
                            g100, g101, g110, g111,
                            u);
}

float noise_2d(vec2 coord, float wavelength)
{
    return noise_2d(coord / wavelength);
}

float noise_3d(vec3 coord, float wavelength)
{
    return noise_3d(coord / wavelength);
}

float dot_noise_2d(vec2 coord, float fractional_max_dot_size, float d_density)
{
    float x = coord.x;
    float y = coord.y;

    float integer_x    = x - fract(x);
    float fractional_x = x - integer_x;

    float integer_y    = y - fract(y);
    float fractional_y = y - integer_y;

    if (rand_2d(vec2(integer_x+1.0, integer_y+1.0)) > d_density) {
        return 0.0;
    }

    float xoffset = rand_2d(vec2(integer_x,     integer_y)) - 0.5;
    float yoffset = rand_2d(vec2(integer_x+1.0, integer_y)) - 0.5;
    float dot_size = 0.5 * fractional_max_dot_size * max(0.25, rand_2d(vec2(integer_x, integer_y + 1.0)));

    vec2 truePos = vec2(0.5 + xoffset * (1.0 - 2.0 * dot_size),
                        0.5 + yoffset * (1.0 - 2.0 * dot_size));

    float dist = length(truePos - vec2(fractional_x, fractional_y));

    return 1.0 - smoothstep(0.3 * dot_size, 1.0 * dot_size, dist);
}

float dot_noise_2d(vec2 coord, float wavelength, float fractional_max_dot_size, float d_density)
{
    return dot_noise_2d(coord / wavelength, fractional_max_dot_size, d_density);
}

float voronoi_noise_2d(vec2 coord, float xrand, float yrand)
{
    float x = coord.x;
    float y = coord.y;

    float integer_x    = x - fract(x);
    float fractional_x = x - integer_x;

    float integer_y    = y - fract(y);
    float fractional_y = y - integer_y;

    float val[4];
    val[0] = rand_2d(vec2(integer_x, integer_y));
    val[1] = rand_2d(vec2(integer_x+1.0, integer_y));
    val[2] = rand_2d(vec2(integer_x, integer_y+1.0));
    val[3] = rand_2d(vec2(integer_x+1.0, integer_y+1.0));

    float xshift[4];
    xshift[0] = xrand * (rand_2d(vec2(integer_x+0.5, integer_y)) - 0.5);
    xshift[1] = xrand * (rand_2d(vec2(integer_x+1.5, integer_y)) -0.5);
    xshift[2] = xrand * (rand_2d(vec2(integer_x+0.5, integer_y+1.0))-0.5);
    xshift[3] = xrand * (rand_2d(vec2(integer_x+1.5, integer_y+1.0))-0.5);

    float yshift[4];
    yshift[0] = yrand * (rand_2d(vec2(integer_x, integer_y +0.5)) - 0.5);
    yshift[1] = yrand * (rand_2d(vec2(integer_x+1.0, integer_y+0.5)) -0.5);
    yshift[2] = yrand * (rand_2d(vec2(integer_x, integer_y+1.5))-0.5);
    yshift[3] = yrand * (rand_2d(vec2(integer_x+1.5, integer_y+1.5))-0.5);

    float dist[4];
    dist[0] = sqrt((fractional_x + xshift[0]) * (fractional_x + xshift[0]) + (fractional_y + yshift[0]) * (fractional_y + yshift[0]));
    dist[1] = sqrt((1.0 -fractional_x + xshift[1]) * (1.0-fractional_x+xshift[1]) + (fractional_y +yshift[1]) * (fractional_y+yshift[1]));
    dist[2] = sqrt((fractional_x + xshift[2]) * (fractional_x + xshift[2]) + (1.0-fractional_y +yshift[2]) * (1.0-fractional_y + yshift[2]));
    dist[3] = sqrt((1.0-fractional_x + xshift[3]) * (1.0-fractional_x + xshift[3]) + (1.0-fractional_y +yshift[3]) * (1.0-fractional_y + yshift[3]));

    int i_min;
    float dist_min = 100.0;
    for (int i = 0; i < 4; ++i) {
        if (dist[i] < dist_min) {
            dist_min = dist[i];
            i_min = i;
        }
    }
    return val[i_min];
}

float voronoi_noise_2d(vec2 coord, float wavelength, float xrand, float yrand)
{
    return voronoi_noise_2d(coord / wavelength, xrand, yrand);
}

float slope_lines_2d(vec2 coord, vec2 s, float steepness)
{
    float x = coord.x;
    float y = coord.y;
    float sx = s.x;
    float sy = s.y;

    float integer_x    = x - fract(x);
    float fractional_x = x - integer_x;

    float integer_y    = y - fract(y);
    float fractional_y = y - integer_y;

    vec2 O = vec2 (0.2 + 0.6 * rand_2d(vec2(integer_x,       integer_y + 1.0)),
                   0.3 + 0.4 * rand_2d(vec2(integer_x + 1.0, integer_y      )));
    vec2 S = vec2( sx, sy);
    vec2 P = vec2(-sy, sx);
    vec2 X = vec2(fractional_x, fractional_y);

    float radius = 0.0 + 0.3 * rand_2d(vec2(integer_x, integer_y));

    float b = (X.y - O.y + O.x * S.y/S.x - X.x * S.y/S.x) / (P.y - P.x * S.y/S.x);
    float a = (X.x - O.x - b * P.x) / S.x;

    return (1.0 - smoothstep(0.7 * (1.0-steepness), 1.2 * (1.0 - steepness),
                             0.6 * abs(a))) * (1.0 - smoothstep(0.0, 1.0 * radius, abs(b)));
}

float slope_lines_2d(vec2 coord, vec2 grad_dir, float wavelength, float steepness)
{
    return slope_lines_2d(coord / wavelength, grad_dir, steepness);
}
