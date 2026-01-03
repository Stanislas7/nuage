$FG_GLSL_VERSION

layout(location = 0) out vec2 fragColor;

in vec2 texcoord;

const uint SAMPLE_COUNT = 1024u;

// math.glsl
float M_2PI();
float sqr(float x);
float pow5(float x);
// hammersley.glsl
vec2 Hammersley(uint i, uint N);

float G1_Smith_GGX(float NdotX, float a)
{
    // a = roughness
    float k = sqr(a) * 0.5;
    return NdotX / (NdotX * (1.0 - k) + k);
}

vec3 importance_sample_GGX(vec2 Xi, float a)
{
    float phi = M_2PI() * Xi.x;
    float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (sqr(a) - 1.0) * Xi.y));
    float sin_theta = sqrt(1.0 - sqr(cos_theta));
    return vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}

vec2 integrate_brdf(float NdotV, float roughness)
{
    vec3 V = vec3(sqrt(1.0 - sqr(NdotV)), 0.0, NdotV);

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 binormal = cross(N, tangent);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H_tangent = importance_sample_GGX(Xi, roughness);
        // From tangent space to world space
        vec3 H = tangent * H_tangent.x + binormal * H_tangent.y + N * H_tangent.z;

        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0) {
            float G = G1_Smith_GGX(NdotV, roughness) * G1_Smith_GGX(NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow5(1.0 - VdotH);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main()
{
    fragColor = integrate_brdf(texcoord.x, texcoord.y);
}
