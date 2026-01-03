$FG_GLSL_VERSION

in VS_OUT {
    float flogz;
    vec2 texcoord;
    vec3 vertex_normal;
    vec4 material_color;
} fs_in;

uniform sampler2D color_tex;

uniform float pbr_metallic;
uniform float pbr_roughness;

// gbuffer_pack.glsl
void gbuffer_pack_pbr_opaque(vec3 normal,
                             vec3 base_color,
                             float metallic,
                             float roughness,
                             float occlusion,
                             vec3 emissive);
// color.glsl
vec3 eotf_inverse_sRGB(vec3 srgb);
// logarithmic_depth.glsl
float logdepth_encode(float z);

void main()
{
    vec3 texel = texture(color_tex, fs_in.texcoord).rgb;
    vec3 color = eotf_inverse_sRGB(texel) * fs_in.material_color.rgb;
    vec3 N = normalize(fs_in.vertex_normal);
    gbuffer_pack_pbr_opaque(N, color, pbr_metallic, pbr_roughness, 1.0, vec3(0.0));
    gl_FragDepth = logdepth_encode(fs_in.flogz);
}
