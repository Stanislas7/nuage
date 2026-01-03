$FG_GLSL_VERSION

layout (quads, fractional_odd_spacing, ccw) in;

in TCS_OUT {
    vec2 p2d_ls;
    vec2 p2d_ws;
    vec2 texcoord;
} tes_in[];

patch in mat4 patch_heights;
patch in vec2 patch_size_inv;

out TES_OUT {
    float flogz;
    vec2 texcoord;
    vec2 p2d_ls;
    vec2 p2d_ws;
    vec3 view_vector;
    vec3 vertex_normal;
    float steepness;
} tes_out;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;
uniform sampler2D landclass;

// Noise Amplitude parameters
uniform vec4 fg_heightAmplitude[128];

const vec4 noise_frequencies = 1.0 / vec4(50.0, 20.0, 10.0, 5.0);

// noise.glsl
vec3 noised_2d(vec2 x);
// logarithmic_depth.glsl
float logdepth_prepare_vs_depth(float z);

/*
 * Bilinearly interpolate between four 2D points according to a weight 'uv'.
 * When uv = [0,0], the bottom left corner p00 is returned. When uv = [1,1],
 * the top right corner p11 is returned.
 */
vec2 bilinear_interp_2d(vec2 p00, vec2 p01, vec2 p10, vec2 p11, vec2 uv)
{
    vec2 a = mix(p00, p10, uv.x);
    vec2 b = mix(p01, p11, uv.x);
    return mix(a, b, uv.y);
}

/*
 * Return the Catmull-Rom basis vector and its derivative for a parameter
 * 't' (0 <= t <= 1).
 *
 * This is used to do cubic interpolation using a Catmull-Rom spline defined by
 * four control points and this basis vector.
 */
void catmull_rom_interp_basis(in float t, out vec4 basis, out vec4 dbasis)
{
    // Catmull-Rom basis matrix for tau=0.5
    const mat4 catmull_rom_basis_M = mat4(0.0, -0.5,  1.0, -0.5,
                                          1.0,  0.0, -2.5,  1.5,
                                          0.0,  0.5,  2.0, -1.5,
                                          0.0,  0.0, -0.5,  0.5);
    float tt = t*t;
    basis  = vec4(1.0,   t,  tt, tt*t) * catmull_rom_basis_M;
    dbasis = vec4(0.0, 1.0, 2*t, 3*tt) * catmull_rom_basis_M;
}

vec2 local_point_at_uv(vec2 uv)
{
    vec2 p00 = tes_in[0].p2d_ls;
    vec2 p01 = tes_in[1].p2d_ls;
    vec2 p10 = tes_in[2].p2d_ls;
    vec2 p11 = tes_in[3].p2d_ls;
    return bilinear_interp_2d(p00, p01, p10, p11, uv);
}

vec2 world_point_at_uv(vec2 uv)
{
    vec2 p00 = tes_in[0].p2d_ws;
    vec2 p01 = tes_in[1].p2d_ws;
    vec2 p10 = tes_in[2].p2d_ws;
    vec2 p11 = tes_in[3].p2d_ws;
    return bilinear_interp_2d(p00, p01, p10, p11, uv);
}

vec2 texcoord_at_uv(vec2 uv)
{
    vec2 t00 = tes_in[0].texcoord;
    vec2 t01 = tes_in[1].texcoord;
    vec2 t10 = tes_in[2].texcoord;
    vec2 t11 = tes_in[3].texcoord;
    return bilinear_interp_2d(t00, t01, t10, t11, uv);
}

void apply_noise(inout float h, inout vec2 dh, in vec2 p, in vec4 noise_amplitudes)
{
    for (int i = 0; i < 4; ++i) {
        float f = noise_frequencies[i];
        float a = noise_amplitudes[i];
        vec3 noise = noised_2d(p * f) - 0.5;
        h += noise.x * a;
        dh += noise.yz * a * f;
    }
}

vec4 height_and_normal_at_uv(vec2 uv, vec2 p, vec4 noise_amplitudes)
{
    vec4 u_basis, ud_basis;
    vec4 v_basis, vd_basis;
    catmull_rom_interp_basis(uv.x, u_basis, ud_basis);
    catmull_rom_interp_basis(uv.y, v_basis, vd_basis);

    mat4 H  = patch_heights;            // Matrix columns are X-axis aligned
    mat4 HT = transpose(patch_heights); // Matrix columns are Y-axis aligned

    vec4 hu;
    hu.x = dot(H[0], u_basis);
    hu.y = dot(H[1], u_basis);
    hu.z = dot(H[2], u_basis);
    hu.w = dot(H[3], u_basis);

    vec4 hv;
    hv.x = dot(HT[0], v_basis);
    hv.y = dot(HT[1], v_basis);
    hv.z = dot(HT[2], v_basis);
    hv.w = dot(HT[3], v_basis);

    float h = dot(hu, v_basis); // Can also be dot(hv, u_basis)

    // We need to take the patch size into account because uv is normalized
    // [0,1] but h is in meters. Scale h down by the reciprocal of the patch
    // width or height.
    float dhdu = dot(hv * patch_size_inv.x, ud_basis);
    float dhdv = dot(hu * patch_size_inv.y, vd_basis);

    // gl_TessCoord uses a left handed reference frame where X points right and
    // Y points up. Our Z-up reference frame is a right handed frame where X
    // points up (North) and Y points right. See makeZUpFrameRelative() in
    // simgear/scene/util/OsgMath.hxx.
    //
    // Calculating the normal from the gradient in the same frame as 'uv':
    //   N = normalize(vec3(-dhdu, -dhdv, 1.0));
    // We swap u and v and negate the new X coordinate to go from one reference
    // frame to another.
    vec2 dh = vec2(-dhdv, dhdu);

    apply_noise(h, dh, p, noise_amplitudes);

    vec3 N = normalize(vec3(-dh, 1.0));
    return vec4(h, N);
}

void main()
{
    vec2 uv = vec2(gl_TessCoord.xy);

    vec2 texcoord = texcoord_at_uv(uv);
    int lc = int(texture(landclass, texcoord).g * 255.0 + 0.5);
    vec4 noise_amplitudes = fg_heightAmplitude[lc];

    vec2 p2d_ls = local_point_at_uv(uv);
    vec2 p2d_ws = world_point_at_uv(uv);

    vec4 hn = height_and_normal_at_uv(uv, p2d_ws, noise_amplitudes);
    vec3 p = vec3(p2d_ls, hn.x);
    vec3 n = hn.yzw;

    float steepness = dot(vec3(0.0, 0.0, 1.0), n);

    gl_Position = osg_ModelViewProjectionMatrix * vec4(p, 1.0);
    tes_out.flogz = logdepth_prepare_vs_depth(gl_Position.w);
    tes_out.texcoord = texcoord;
    tes_out.p2d_ls = p2d_ls;
    tes_out.p2d_ws = p2d_ws;
    tes_out.view_vector = vec3(osg_ModelViewMatrix * vec4(p, 1.0));
    tes_out.vertex_normal = osg_NormalMatrix * n;
    tes_out.steepness = steepness;
}
