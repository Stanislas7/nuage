$FG_GLSL_VERSION

#pragma import_defines(FG_MVR_CELLS)

// for FG_VIEW_ID
FG_VIEW_GLOBAL

uniform sampler3D aerial_perspective_tex;

const vec2 ap_slice_size = vec2(64.0, 64.0);
const vec2 ap_inv_slice_size = 1.0 / ap_slice_size;
const float ap_slice_count = 32.0;
const float ap_inv_slice_count = 1.0 / ap_slice_count;
const float ap_m_per_slice = 4000.0;
const float ap_inv_m_per_slice = 1.0 / ap_m_per_slice;

// mvr.vert / mvr.frag / mvr_multipass.glsl
vec2 mvr_raw_texcoord_transform_buf(vec2 raw_texcoord);

uvec3 ap_get_voxel_view_offset()
{
    uvec3 ret = uvec3(0);
    if (FG_MVR_CELLS > 1)
        ret.x += FG_VIEW_ID*64;
    return ret;
}

vec2 ap_get_uv_for_voxel(uvec2 voxel)
{
    return (vec2(voxel) + 0.5) * ap_inv_slice_size;
}

float ap_slice_to_depth(float slice)
{
    return slice * ap_m_per_slice;
}

float ap_depth_to_slice(float depth)
{
    return depth * ap_inv_m_per_slice;
}

float ap_apply_squared_distribution(float slice)
{
    slice *= ap_inv_slice_count;
    slice *= slice;
    slice *= ap_slice_count;
    return slice;
}

float ap_undo_squared_distribution(float slice)
{
    slice *= ap_inv_slice_count;
    slice = sqrt(slice);
    slice *= ap_slice_count;
    return slice;
}

vec4 get_aerial_perspective(vec2 raw_coord, vec3 P)
{
    float depth = abs(P.z);
    float slice = ap_depth_to_slice(depth);
    slice = ap_undo_squared_distribution(slice);
    vec2 coord = mvr_raw_texcoord_transform_buf(raw_coord);

    vec4 ap;
    if (slice < 1.0) {
        ap = mix(
            vec4(0.0, 0.0, 0.0, 1.0),
            texture(aerial_perspective_tex, vec3(coord, 0.0)),
            slice);
    } else {
        float w = (slice - 1.0) / (ap_slice_count - 1.0);
        ap = texture(aerial_perspective_tex, vec3(coord, w));
    }
    return ap;
}

vec3 mix_aerial_perspective(vec3 color, vec4 ap)
{
    return color * ap.a + ap.rgb;
}

vec3 add_aerial_perspective(vec3 color, vec2 raw_coord, vec3 P)
{
    return mix_aerial_perspective(color, get_aerial_perspective(raw_coord, P));
}
