$FG_GLSL_VERSION

/*
 * Create a cotangent frame without a pre-computed tangent basis.
 * "Normal Mapping Without Precomputed Tangents" by Christian Schüler (2013).
 * http://www.thetenthplanet.de/archives/1180
 */
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    // solve the linear system
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    // construct a scale-invariant frame
    float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
    return mat3(T * invmax, B * invmax, N);
}

/*
 * Perturb the interpolated vertex normal N according to a normal map.
 * V is the view vector (eye to vertex, not normalized). Both N and V must be
 * in the same space.
 */
vec3 perturb_normal(vec3 N, vec3 V, vec2 texcoord, sampler2D tex)
{
    vec3 normal = texture(tex, texcoord).rgb;
    // Sign expansion because the normal map format is unsigned
    normal = normal * 255.0 / 127.0 - 128.0 / 127.0;
    mat3 TBN = cotangent_frame(N, V, texcoord);
    return normalize(TBN * normal);
}

/*
 * Perturb the interpolated vertex normal N according to a given height value.
 * This height value can come from a height map, or from procedural functions.
 * V is the vertex position in view space. Both N and V must be in the same space.
 * "Bump Mapping Unparametrized Surfaces on the GPU" by Morten S. Mikkelsen (2010).
 */
vec3 perturb_normal_from_height(vec3 N, vec3 V, float height)
{
    vec3 dpdx = dFdx(V);
    vec3 dpdy = dFdy(V);
    float dhdx = dFdx(height);
    float dhdy = dFdy(height);
    vec3 r1 = cross(dpdy, N);
    vec3 r2 = cross(N, dpdx);
    vec3 surf_grad = (r1 * dhdx + r2 * dhdy) / dot(dpdx, r1);
    return normalize(N - surf_grad);
}
