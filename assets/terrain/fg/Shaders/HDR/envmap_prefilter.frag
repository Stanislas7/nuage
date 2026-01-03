$FG_GLSL_VERSION
/*
 * Mostly based on 'Moving Frostbite to Physically Based Rendering'
 * https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
 */

layout(location = 0) out vec3 fragColor0;
layout(location = 1) out vec3 fragColor1;
layout(location = 2) out vec3 fragColor2;
layout(location = 3) out vec3 fragColor3;
layout(location = 4) out vec3 fragColor4;
layout(location = 5) out vec3 fragColor5;

in vec3 cubemap_coord0;
in vec3 cubemap_coord1;
in vec3 cubemap_coord2;
in vec3 cubemap_coord3;
in vec3 cubemap_coord4;
in vec3 cubemap_coord5;

uniform samplerCube envmap_tex;
uniform float roughness;
uniform int num_samples;
uniform int mip_count;

// math.glsl
float M_PI();
float M_2PI();
float M_4PI();
float sqr(float x);
// hammersley.glsl
vec2 Hammersley(uint i, uint N);

vec3 importance_sample_GGX(vec2 Xi, float a)
{
    float phi = M_2PI() * Xi.x;
    float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (sqr(a) - 1.0) * Xi.y));
    float sin_theta = sqrt(1.0 - sqr(cos_theta));
    return vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}

float D_GGX(float NdotH, float a2)
{
    float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (M_PI() * f * f);
}

vec3 prefilter(vec3 N)
{
    vec3 V = N; // n = v simplification
    float a = sqr(roughness);
    float envmap_size = float(textureSize(envmap_tex, 0).x);

    vec3 result = vec3(0.0);
    float weight = 0.0;

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 binormal = cross(N, tangent);

    uint sample_count = uint(num_samples);

    for (uint i = 0u; i < sample_count; ++i) {
        vec2 Xi = Hammersley(i, sample_count);
        vec3 H_tangent = importance_sample_GGX(Xi, a);
        // From tangent space to world space
        vec3 H = tangent * H_tangent.x + binormal * H_tangent.y + N * H_tangent.z;

        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            float NdotH = clamp(dot(N, H), 0.0, 1.0);
            float VdotH = clamp(dot(V, H), 0.0, 1.0);

            float pdf = D_GGX(NdotH, a) * NdotH / (4.0 * VdotH);
            float omega_s = 1.0 / (float(sample_count) * pdf);
            float omega_p = M_4PI() / (6.0 * envmap_size * envmap_size);
            float miplevel = clamp(0.5 * log2(omega_s / omega_p) + 1.0,
                                   0.0, float(mip_count));

            result += textureLod(envmap_tex, L, miplevel).rgb * NdotL;
            weight += NdotL;
        }
    }

    return result / weight;
}

void main()
{
    fragColor0 = prefilter(normalize(cubemap_coord0));
    fragColor1 = prefilter(normalize(cubemap_coord1));
    fragColor2 = prefilter(normalize(cubemap_coord2));
    fragColor3 = prefilter(normalize(cubemap_coord3));
    fragColor4 = prefilter(normalize(cubemap_coord4));
    fragColor5 = prefilter(normalize(cubemap_coord5));
}
