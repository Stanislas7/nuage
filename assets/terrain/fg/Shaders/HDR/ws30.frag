$FG_GLSL_VERSION

in TES_OUT {
    float flogz;
    vec2 texcoord;
    vec2 p2d_ls;
    vec2 p2d_ws;
    vec3 view_vector;
    vec3 vertex_normal;
    float steepness;
} fs_in;

// Passed from VPBTechnique, not the Effect
uniform sampler2D landclass;
uniform sampler2DArray textureArray;
uniform float fg_tileWidth;
uniform float fg_tileHeight;
uniform vec4 fg_dimensionsArray[128];
uniform vec4 fg_textureLookup1[128];
uniform vec4 fg_textureLookup2[128];

// PBR parameters
uniform vec4 fg_materialPBRParams[128];
uniform vec4 fg_materialPBREmission[128];

// Noise Amplitude parameters
uniform vec4 fg_bumpmapAmplitude[128];

// math.glsl
float pow4(float x);
// color.glsl
vec3 eotf_inverse_sRGB(vec3 srgb);
// noise.glsl
float noise_2d(vec2 coord, float wavelength);
// normalmap.glsl
vec3 perturb_normal_from_height(vec3 N, vec3 V, float height);
// gbuffer_pack.glsl
void gbuffer_pack_pbr_opaque(vec3 normal,
                             vec3 base_color,
                             float metallic,
                             float roughness,
                             float occlusion,
                             vec3 emissive);
// logarithmic_depth.glsl
float logdepth_encode(float z);

// A fade function for procedural scales which are smaller than a pixel
float detail_fade(float scale, float angle, float steepness_factor, float dist)
{
    float fade_dist = 2000.0 * scale * angle * steepness_factor;
    return 1.0 - smoothstep(0.5 * fade_dist, fade_dist, dist);
}

void main()
{
    int lc = int(texture(landclass, fs_in.texcoord).g * 255.0 + 0.5);

    float frag_dist = length(fs_in.view_vector);

    vec3 N = normalize(fs_in.vertex_normal);

    // We need to fade procedural structures when they get smaller than a single
    // pixel. For this we need to know under what angle we see the surface.
    float view_angle = clamp(dot(N, normalize(-fs_in.view_vector)), 0.0, 1.0);
    // We also need a factor dependent on the steepness of the surface
    float steepness_factor = 1.0 / max(pow4(fs_in.steepness), 0.1);

    vec4 noise_vec = vec4(
        noise_2d(fs_in.p2d_ls, 2.0) * detail_fade(2.0, view_angle, steepness_factor, frag_dist),
        noise_2d(fs_in.p2d_ls, 1.0) * detail_fade(1.0, view_angle, steepness_factor, frag_dist),
        noise_2d(fs_in.p2d_ls, 0.5) * detail_fade(0.5, view_angle, steepness_factor, frag_dist),
        noise_2d(fs_in.p2d_ls, 0.2) * detail_fade(0.2, view_angle, steepness_factor, frag_dist));
    noise_vec *= fg_bumpmapAmplitude[lc];

    float displacement = noise_vec.x + noise_vec.y + noise_vec.z + noise_vec.w;
    N = perturb_normal_from_height(N, fs_in.view_vector, displacement);

    // TODO: Add the landclass search functions
    // The Landclass for this particular fragment.  This can be used to
    // index into the atlas textures.
    uint tex1 = uint(fg_textureLookup1[lc].r * 255.0 + 0.5);

    // Different textures have different dimensions.
    vec2 atlas_dimensions = fg_dimensionsArray[lc].st;
    vec2 atlas_scale =  vec2(fg_tileWidth / atlas_dimensions.s, fg_tileHeight / atlas_dimensions.t );
    vec2 st = atlas_scale * fs_in.texcoord;

    vec3 texel = eotf_inverse_sRGB(texture(textureArray, vec3(st, tex1)).rgb);

    vec3 HDRParams = fg_materialPBRParams[lc].xyz;
    vec3 emission = fg_materialPBREmission[lc].xyz;

    gbuffer_pack_pbr_opaque(N, texel, HDRParams[0], HDRParams[1], HDRParams[2], emission);
    gl_FragDepth = logdepth_encode(fs_in.flogz);
}
