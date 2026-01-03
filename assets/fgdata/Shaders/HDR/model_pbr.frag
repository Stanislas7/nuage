$FG_GLSL_VERSION

in VS_OUT {
    float flogz;
    vec2 texcoord;
    vec3 vertex_normal;
    vec3 view_vector;
} fs_in;

uniform sampler2D base_color_tex;
uniform sampler2D normal_tex;
uniform sampler2D orm_tex;
uniform sampler2D emissive_tex;

uniform vec4 base_color_factor;
uniform float metallic_factor;
uniform float roughness_factor;
uniform vec3 emissive_factor;

// gbuffer_pack.glsl
void gbuffer_pack_pbr_opaque(vec3 normal,
                             vec3 base_color,
                             float metallic,
                             float roughness,
                             float occlusion,
                             vec3 emissive);
// color.glsl
vec3 eotf_inverse_sRGB(vec3 srgb);
// normalmap.glsl
vec3 perturb_normal(vec3 N, vec3 V, vec2 texcoord, sampler2D tex);
// logarithmic_depth.glsl
float logdepth_encode(float z);

void main()
{
    vec4 base_color_texel = texture(base_color_tex, fs_in.texcoord);
    vec3 base_color = eotf_inverse_sRGB(base_color_texel.rgb) * base_color_factor.rgb;
    // Ignore alpha in base color. We assume this is a completely opaque object

    vec3 orm = texture(orm_tex, fs_in.texcoord).rgb;
    float occlusion = orm.r;
    float roughness = orm.g * roughness_factor;
    float metallic = orm.b * metallic_factor;

    vec3 emissive_texel = texture(emissive_tex, fs_in.texcoord).rgb;
    vec3 emissive = eotf_inverse_sRGB(emissive_texel) * emissive_factor;

    vec3 N = normalize(fs_in.vertex_normal);
    N = perturb_normal(N, fs_in.view_vector, fs_in.texcoord, normal_tex);

    gbuffer_pack_pbr_opaque(N, base_color, metallic, roughness, occlusion, emissive);
    gl_FragDepth = logdepth_encode(fs_in.flogz);
}
