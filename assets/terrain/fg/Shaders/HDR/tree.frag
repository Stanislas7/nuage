$FG_GLSL_VERSION

in VS_OUT {
    float flogz;
    vec2 texcoord;
    vec3 vs_up;
    vec3 vs_pos;
    vec3 vs_tree_pos;
    float autumn_flag;
    float scale;
} fs_in;

uniform sampler2D color_tex;
uniform sampler2D normal_tex;

uniform float cseason;

// gbuffer_pack.glsl
void gbuffer_pack_pbr_dither(vec3 normal,
                             vec3 base_color,
                             float metallic,
                             float roughness,
                             float occlusion,
                             vec3 emissive,
                             float dither);
// color.glsl
vec3 eotf_inverse_sRGB(vec3 srgb);
// dither.glsl
float dither_checkerboard(vec2 pixel_coord, float t);
// logarithmic_depth.glsl
float logdepth_encode(float z);

/*
 * Ray-cylinder intersection
 * The cylinder is positioned at pa, is infinite, and is aligned with axis va.
 * The x component of the return value contains the distance to the
 * intersection or -1.0 if no intersection was found, and the yzw components
 * contain the normal at the intersection.
 * https://www.shadertoy.com/view/4lcSRn
 */
vec4 ray_cylinder_intersect(vec3 ro, vec3 rd, vec3 pa, vec3 va, float ra)
{
    vec3 oc = ro - pa;
    float vava = dot(va,va);
    float vard = dot(va,rd);
    float vaoc = dot(va,oc);
    float k2 = vava            - vard*vard;
    float k1 = vava*dot(oc,rd) - vaoc*vard;
    float k0 = vava*dot(oc,oc) - vaoc*vaoc - ra*ra*vava;
    float h = k1*k1 - k2*k0;
    if (h < 0.0) return vec4(-1.0);
    h = sqrt(h);
    float t = (-k1-h) / k2;
    float y = vaoc + t*vard;
    return vec4(t, (oc + t*rd - va*y / vava) / ra);
}

void main()
{
    vec4 texel = texture(color_tex, fs_in.texcoord);
    if (dither_checkerboard(gl_FragCoord.xy, texel.a) < 1.0) {
        discard;
        return;
    }

    // Seasonal color changes
    if (cseason < 1.5 && fs_in.autumn_flag > 0.0) {
        texel.r = min(1.0, (1.0 + 5.0 * cseason * fs_in.autumn_flag) * texel.r);
        texel.b = max(0.0, (1.0 - 8.0 * cseason) * texel.b);
    }

    vec3 color = eotf_inverse_sRGB(texel.rgb);

    vec3 ray_dir = normalize(fs_in.vs_pos);

    vec4 intersect = ray_cylinder_intersect(
        vec3(0.0), ray_dir, fs_in.vs_tree_pos, fs_in.vs_up, fs_in.scale);
    vec3 normal = intersect.yzw;
    vec3 tangent = normalize(cross(fs_in.vs_up, normal));

    vec3 N = texture(normal_tex, fs_in.texcoord).rgb;
    // Back facing quads are horizontally mirrored
    if (!gl_FrontFacing) {
        N.x = 1.0 - N.x;
    }
    N = N * 2.0 - 1.0;
    // This is exact only for viewing under 90 degrees
    N = normalize(N.x * tangent + N.y * fs_in.vs_up + N.z * normal);

    gbuffer_pack_pbr_dither(N, color, 0.0, 1.0, 1.0, vec3(0.0), texel.a);
    gl_FragDepth = logdepth_encode(fs_in.flogz);
}
