$FG_GLSL_VERSION

uniform sampler2DArray textureArray;
uniform vec3 sea_color;

uniform float osg_SimulationTime;

// Hardcoded indexes into the texture atlas
const int ATLAS_INDEX_WATER                 = 0;
const int ATLAS_INDEX_WATER_REFLECTION      = 1;
const int ATLAS_INDEX_WAVES_VERT10_NM       = 2;
const int ATLAS_INDEX_WATER_SINE_NMAP       = 3;
const int ATLAS_INDEX_WATER_REFLECTION_GREY = 4;
const int ATLAS_INDEX_SEA_FOAM              = 5;
const int ATLAS_INDEX_PERLIN_NOISE_NM       = 6;
const int ATLAS_INDEX_OCEAN_DEPTH           = 7;
const int ATLAS_INDEX_GLOBAL_COLORS         = 8;
const int ATLAS_INDEX_PACKICE_OVERLAY       = 9;

// normalmap.glsl
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv);

// Get a 2D rotation matrix from a given angle
void get_2d_rot_mat(in float angle, out mat2 rotmat)
{
    rotmat = mat2(cos(angle), -sin(angle),
                  sin(angle),  cos(angle));
}

/*
 * Perturb the (flat) water surface normal according to several normalmaps.
 */
vec3 ws30_perturb_water_normal(vec3 N, vec3 V, vec2 uv)
{
    const vec2 sca         = vec2(0.005);
    const vec2 sca2        = vec2(0.02);
    const vec2 tscale      = vec2(0.25);
    const float scale      = 5.0;
    const float mix_factor = 0.5;

    vec2 st = vec2(uv * tscale) * scale;
    vec2 disdis = texture(textureArray, vec3(st, ATLAS_INDEX_WATER_SINE_NMAP)).rg * 2.0 - 1.0;

    st = vec2(uv + disdis * sca2) * scale;
    vec3 N0 = vec3(texture(textureArray, vec3(st, ATLAS_INDEX_WAVES_VERT10_NM)) * 2.0 - 1.0);
    st = vec2(uv + disdis * sca) * scale;
    vec3 N1 = vec3(texture(textureArray, vec3(st, ATLAS_INDEX_PERLIN_NOISE_NM)) * 2.0 - 1.0);

    st = vec2(uv * tscale) * scale;
    N0 += vec3(texture(textureArray, vec3(st, ATLAS_INDEX_WAVES_VERT10_NM)) * 2.0 - 1.0);
    N1 += vec3(texture(textureArray, vec3(st, ATLAS_INDEX_PERLIN_NOISE_NM)) * 2.0 - 1.0);

    mat2 rotmatrix;
    get_2d_rot_mat(radians(2.0 * sin(osg_SimulationTime * 0.005)), rotmatrix);
    st = vec2(uv * rotmatrix * (tscale + sca2)) * scale;
    N0 += vec3(texture(textureArray, vec3(st, ATLAS_INDEX_WAVES_VERT10_NM)) * 2.0 - 1.0);
    N1 += vec3(texture(textureArray, vec3(st, ATLAS_INDEX_PERLIN_NOISE_NM)) * 2.0 - 1.0);

    get_2d_rot_mat(radians(-4.0 * sin(osg_SimulationTime * 0.003)), rotmatrix);
    st = vec2(uv * rotmatrix + disdis * sca2) * scale;
    N0 += vec3(texture(textureArray, vec3(st, ATLAS_INDEX_WAVES_VERT10_NM)) * 2.0 - 1.0);
    st = vec2(uv * rotmatrix + disdis * sca)  * scale;
    N1 += vec3(texture(textureArray, vec3(st, ATLAS_INDEX_PERLIN_NOISE_NM)) * 2.0 - 1.0);

    vec3 normal = normalize(mix(N0, N1, mix_factor));

    mat3 TBN = cotangent_frame(N, V, uv);
    return normalize(TBN * normal);
}

vec3 ws30_get_water_color()
{
    return sea_color;
}
