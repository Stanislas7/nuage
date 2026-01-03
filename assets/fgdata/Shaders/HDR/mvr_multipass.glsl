$FG_GLSL_VERSION

#pragma import_defines(FG_MVR_CELLS)

// for FG_VIEW_ID
FG_VIEW_GLOBAL

vec2 mvr_raw_texcoord_transform_buf(vec2 raw_texcoord)
{
#if FG_MVR_CELLS > 1
    raw_texcoord.x = (raw_texcoord.x + FG_VIEW_ID)/FG_MVR_CELLS;
#endif
    return raw_texcoord;
}

vec2 mvr_raw_texcoord_transform_fb(vec2 raw_texcoord)
{
    // Not actually the same thing, but as long as the views are the same size
    // it'll do for now.
    return mvr_raw_texcoord_transform_buf(raw_texcoord);
}
