$FG_GLSL_VERSION

layout(location = 0) out vec4 out_gbuffer0;
layout(location = 1) out vec4 out_gbuffer1;
layout(location = 2) out vec4 out_gbuffer2;
layout(location = 3) out vec3 out_gbuffer3;

// normal_encoding.glsl
vec2 encode_normal(vec3 n);

float mat_id_to_float(uint mat_id)
{
    return float(mat_id) * (1.0 / 3.0);
}

/*
 * Write the given surface properties to the G-buffer.
 * See the HDR pipeline definition in $FG_ROOT/Compositor/HDR/hdr.xml for the
 * G-buffer layout.
 */
void gbuffer_pack(vec3 normal,
                  vec3 base_color,
                  float metallic,
                  float roughness,
                  float occlusion,
                  vec3 emissive,
                  float dither,
                  uint mat_id,
                  vec2 mat_specific)
{
    out_gbuffer0.rg  = encode_normal(normal);
    out_gbuffer0.b   = roughness;
    out_gbuffer0.a   = mat_id_to_float(mat_id);
    out_gbuffer1.rgb = base_color;
    out_gbuffer1.a   = dither;
    out_gbuffer2.rg  = mat_specific;
    out_gbuffer2.b   = metallic;
    out_gbuffer2.a   = occlusion;
    out_gbuffer3.rgb = emissive;
}

/*
 * Write a standard PBR opaque material to the G-Buffer.
 * mat_id=3 and no material specific params.
 */
void gbuffer_pack_pbr_opaque(vec3 normal,
                             vec3 base_color,
                             float metallic,
                             float roughness,
                             float occlusion,
                             vec3 emissive)
{
    gbuffer_pack(normal, base_color, metallic, roughness, occlusion, emissive,
                 0.0, 3u, vec2(0.0));
}

/*
 * Write a dithered standard PBR material to the G-Buffer.
 * mat_id=3 and no material specific params.
 */
void gbuffer_pack_pbr_dither(vec3 normal,
                             vec3 base_color,
                             float metallic,
                             float roughness,
                             float occlusion,
                             vec3 emissive,
                             float dither)
{
    gbuffer_pack(normal, base_color, metallic, roughness, occlusion, emissive,
                 dither, 3u, vec2(0.0));
}

/*
 * Write a water fragment to the G-Buffer.
 */
void gbuffer_pack_water(vec3 normal,
                        vec3 floor_color)
{
    out_gbuffer0.rg  = encode_normal(normal);
    out_gbuffer1.rgb = floor_color;
    out_gbuffer1.a   = 0.0;
    out_gbuffer0.a   = mat_id_to_float(2u);
}
