$FG_GLSL_VERSION

#pragma import_defines(BACKDROP_COLOR, SHADOW, OUTLINE)
#pragma import_defines(SIGNED_DISTANCE_FIELD, TEXTURE_DIMENSION, GLYPH_DIMENSION)

// GL_ALPHA and GL_LUMINANCE_ALPHA are deprecated in GL3/GL4 core profile,
// use GL_RED & GL_RB in this case.
// See osgText/Glyph.cpp
#if 1
#  define ALPHA r
#  define SDF g
#else
#  define ALPHA a
#  define SDF r
#endif

out vec4 fragColor;

in VS_OUT {
    vec2 texcoord;
    vec4 vertex_color;
} fs_in;

uniform sampler2D glyphTexture;

#ifndef TEXTURE_DIMENSION
const float TEXTURE_DIMENSION = 1024.0;
#endif

#ifndef GLYPH_DIMENSION
const float GLYPH_DIMENSION = 32.0;
#endif

#ifdef SIGNED_DISTANCE_FIELD

float distanceFromEdge(vec2 tc)
{
    float center_alpha = textureLod(glyphTexture, tc, 0.0).SDF;
    if (center_alpha==0.0) return -1.0;

    //float distance_scale = (1.0/4.0)*1.41;
    float distance_scale = (1.0/6.0)*1.41;
    //float distance_scale = (1.0/8.0)*1.41;

    return (center_alpha-0.5)*distance_scale;
}

#ifdef OUTLINE
vec2 colorCoeff(float edge_distance, float blend_width, float blend_half_width)
{
    float outline_width = OUTLINE * 0.5;
    if (edge_distance > blend_half_width) {
        return vec2(1.0, 0.0);
    } else if (edge_distance > -blend_half_width) {
        float f = smoothstep(0.0, 1.0, (blend_half_width-edge_distance)/(blend_width));
        return vec2(1.0 - f, f);
    } else if (edge_distance > (blend_half_width - outline_width)) {
        return vec2(0.0, 1.0);
    } else if (edge_distance > -(outline_width + blend_half_width)) {
        return vec2(0.0, smoothstep(0.0, 1.0, (blend_half_width + outline_width + edge_distance) / blend_width));
    } else {
        return vec2(0.0, 0.0);
    }
}
#else
float colorCoeff(float edge_distance, float blend_width, float blend_half_width)
{
    if (edge_distance>blend_half_width) {
        return 1.0;
    } else if (edge_distance>-blend_half_width) {
        return smoothstep(1.0, 0.0, (blend_half_width-edge_distance)/(blend_width));
    } else {
        return 0.0;
    }
}
#endif

vec4 textColor(vec2 src_texcoord)
{
    float sample_distance_scale = 0.75;
    vec2 dx = dFdx(src_texcoord)*sample_distance_scale;
    vec2 dy = dFdy(src_texcoord)*sample_distance_scale;

    float distance_across_pixel = length(dx+dy)*(TEXTURE_DIMENSION/GLYPH_DIMENSION);

    // compute the appropriate number of samples required to avoid aliasing.
    int maxNumSamplesAcrossSide = 4;

    int numSamplesX = int(TEXTURE_DIMENSION * length(dx));
    int numSamplesY = int(TEXTURE_DIMENSION * length(dy));
    if (numSamplesX < 2) numSamplesX = 2;
    if (numSamplesY < 2) numSamplesY = 2;
    if (numSamplesX > maxNumSamplesAcrossSide) numSamplesX = maxNumSamplesAcrossSide;
    if (numSamplesY > maxNumSamplesAcrossSide) numSamplesY = maxNumSamplesAcrossSide;

    vec2 delta_tx = dx/float(numSamplesX-1);
    vec2 delta_ty = dy/float(numSamplesY-1);

    float numSamples = float(numSamplesX)*float(numSamplesY);
    float blend_width = 1.5*distance_across_pixel/numSamples;
    float blend_half_width = blend_width*0.5;

    // check whether fragment is wholly within or outwith glyph body+outline
    float cd = distanceFromEdge(src_texcoord); // central distance (distance from center to edge)
    if (cd - blend_half_width > distance_across_pixel) return fs_in.vertex_color; // pixel fully within glyph body

#ifdef OUTLINE
    float outline_width = OUTLINE * 0.5;
    if ((-cd - outline_width - blend_half_width) > distance_across_pixel)
        return vec4(0.0); // pixel fully outside outline+glyph body
    vec2 color_coeff = vec2(0.0, 0.0);
#else
    if (-cd - blend_half_width > distance_across_pixel)
        return vec4(0.0); // pixel fully outside glyph body
    float color_coeff = 0.0;
#endif

    // use multi-sampling to provide high quality antialised fragments
    vec2 origin = src_texcoord - dx*0.5 - dy*0.5;
    for(;numSamplesY>0; --numSamplesY) {
        vec2 pos = origin;
        int numX = numSamplesX;
        for(;numX>0; --numX) {
#ifdef OUTLINE
            color_coeff += colorCoeff(distanceFromEdge(pos), blend_width, blend_half_width);
#else
            color_coeff += colorCoeff(distanceFromEdge(pos), blend_width, blend_half_width);
#endif
            pos += delta_tx;
        }
        origin += delta_ty;
    }
    color_coeff /= numSamples;

#ifdef OUTLINE
    float vertex_alpha = fs_in.vertex_color.a * color_coeff.x;
    float outline_alpha = BACKDROP_COLOR.a * color_coeff.y;
    float total_alpha = vertex_alpha + outline_alpha;

    if (total_alpha <= 0.0)
        return vec4(0.0, 0.0, 0.0, 0.0);

    return vec4(fs_in.vertex_color.rgb * (vertex_alpha / total_alpha)
                + BACKDROP_COLOR.rgb * (outline_alpha / total_alpha), total_alpha);
#else
    return vec4(fs_in.vertex_color.rgb, fs_in.vertex_color.a * color_coeff);
#endif
}

#else

vec4 textColor(vec2 src_texcoord)
{
#ifdef OUTLINE
    float alpha = texture(glyphTexture, src_texcoord).ALPHA;
    float delta_tc = 1.6 * OUTLINE * GLYPH_DIMENSION / TEXTURE_DIMENSION;

    float outline_alpha = alpha;
    vec2 origin = src_texcoord - vec2(delta_tc*0.5, delta_tc*0.5);

    float numSamples = 3.0;
    delta_tc = delta_tc / (numSamples - 1.0);

    float background_alpha = 1.0;

    for(float i=0.0; i < numSamples; ++i) {
        for(float j=0.0; j < numSamples; ++j) {
            float local_alpha = texture(glyphTexture, origin + vec2(i*delta_tc, j*delta_tc)).ALPHA;
            outline_alpha = max(outline_alpha, local_alpha);
            background_alpha = background_alpha * (1.0 - local_alpha);
        }
    }

    float mipmapLevel = textureQueryLod(glyphTexture, src_texcoord).x;
    if (mipmapLevel < 1.0) {
        outline_alpha = mix(1.0 - background_alpha, outline_alpha, mipmapLevel / 1.0);
    }

    if (outline_alpha < alpha) outline_alpha = alpha;
    if (outline_alpha > 1.0) outline_alpha = 1.0;

    if (outline_alpha == 0.0) return vec4(0.0, 0.0, 0.0, 0.0); // outside glyph and outline

    vec4 color = mix(BACKDROP_COLOR, fs_in.vertex_color, smoothstep(0.0, 1.0, alpha));
    color.a = fs_in.vertex_color.a * smoothstep(0.0, 1.0, outline_alpha);

    return color;
#else
    float alpha = texture(glyphTexture, src_texcoord).ALPHA;
    if (alpha == 0.0) vec4(0.0, 0.0, 0.0, 0.0);
    return vec4(fs_in.vertex_color.rgb, fs_in.vertex_color.a * alpha);
#endif
}
#endif

void main()
{
    if (fs_in.texcoord.x < 0.0 && fs_in.texcoord.y < 0.0) {
        fragColor = fs_in.vertex_color;
        return;
    }

#ifdef SHADOW
    float scale = -1.0 * GLYPH_DIMENSION / TEXTURE_DIMENSION;
    vec2 delta_tc = SHADOW * scale;
    vec4 shadow_color = textColor(fs_in.texcoord + delta_tc);
    shadow_color.rgb = BACKDROP_COLOR.rgb;

    vec4 glyph_color = textColor(fs_in.texcoord);

    // lower the alpha_power value the greater the staturation, no need to be so aggressive with SDF than GREYSCALE
#if SIGNED_DISTANCE_FIELD
    float alpha_power = 0.6;
#else
    float alpha_power = 0.5;
#endif

    // over saturate the alpha values to make sure the font and its shadow are clear
    shadow_color.a = pow(shadow_color.a, alpha_power);
    glyph_color.a = pow(glyph_color.a, alpha_power);
    vec4 color = mix(shadow_color, glyph_color, glyph_color.a);
#else
    vec4 color = textColor(fs_in.texcoord);
#endif

    if (color.a == 0.0) discard;

    fragColor = color;
}
