$FG_GLSL_VERSION

in VS_OUT {
    float flogz;
    vec2 texcoord;
    vec2 orthophoto_texcoord;
    vec3 vertex_normal;
} fs_in;

uniform sampler2D color_tex;
uniform sampler2D orthophoto_tex;

uniform bool orthophotoAvailable;

const float TERRAIN_METALLIC  = 0.0;
const float TERRAIN_ROUGHNESS = 0.95;

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
    if (orthophotoAvailable) {
        vec4 sat_texel = texture(orthophoto_tex, fs_in.orthophoto_texcoord);
        if (sat_texel.a > 0.0) {
            texel.rgb = sat_texel.rgb;
        }
    }

    vec3 color = eotf_inverse_sRGB(texel);

    vec3 N = normalize(fs_in.vertex_normal);

    gbuffer_pack_pbr_opaque(N, color, TERRAIN_METALLIC, TERRAIN_ROUGHNESS, 1.0, vec3(0.0));
    gl_FragDepth = logdepth_encode(fs_in.flogz);
}
