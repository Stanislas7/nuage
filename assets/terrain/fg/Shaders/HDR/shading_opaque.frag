$FG_GLSL_VERSION

layout(location = 0) out vec3 fragColor;

in vec2 raw_texcoord; // QUAD_TEXCOORD_RAW
in vec2 texcoord;

uniform sampler2D gbuffer0_tex;
uniform sampler2D gbuffer1_tex;
uniform sampler2D gbuffer2_tex;
uniform sampler2D gbuffer3_tex;
uniform sampler2D depth_tex;

FG_VIEW_GLOBAL
uniform mat4 fg_ViewMatrixInverse[FG_NUM_VIEWS];
uniform mat4 fg_ProjectionMatrix[FG_NUM_VIEWS];

// normal_encoding.glsl
vec3 decode_normal(vec2 f);
// shading_opaque.glsl
vec3 eval_lights(vec3 base_color,
                 float metallic,
                 float roughness,
                 float occlusion,
                 vec3 emissive,
                 vec3 P, vec3 N, vec3 V,
                 vec2 uv, vec2 raw_uv,
                 mat4 view_matrix_inverse,
                 mat4 projection_matrix);
// pos_from_depth.glsl
vec3 get_view_space_from_depth(vec2 uv, float depth);

void main()
{
    float depth = texture(depth_tex, texcoord).r;
    // Ignore the background
    if (depth == 1.0) {
        discard;
        return;
    }

    // Sample the G-Buffer
    vec4 gbuffer0 = texture(gbuffer0_tex, texcoord);
    // We only care about the standard PBR material (mat_id=3).
    // Discard all other fragments.
    uint mat_id = uint(gbuffer0.a * 3.0);
    if (!(mat_id == 3u || mat_id == 0u)) {
        discard;
        return;
    }

    vec4 gbuffer1 = texture(gbuffer1_tex, texcoord);
    vec4 gbuffer2 = texture(gbuffer2_tex, texcoord);
    vec3 gbuffer3 = texture(gbuffer3_tex, texcoord).rgb;

    // Unpack the G-Buffer
    vec3  N          = decode_normal(gbuffer0.rg);
    float roughness  = gbuffer0.b;
    vec3  base_color = gbuffer1.rgb;
    float metallic   = gbuffer2.b;
    float occlusion  = gbuffer2.a;
    vec3  emissive   = gbuffer3.rgb;

    vec3 P = get_view_space_from_depth(texcoord, depth);
    vec3 V = normalize(-P);

    fragColor = eval_lights(base_color,
                            metallic,
                            roughness,
                            occlusion,
                            emissive,
                            P, N, V,
                            texcoord, raw_texcoord,
                            fg_ViewMatrixInverse[FG_VIEW_ID],
                            fg_ProjectionMatrix[FG_VIEW_ID]);
}
