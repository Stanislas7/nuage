$FG_GLSL_VERSION
/*
 * Thrust flame shader
 * Original ALS (Atmospheric Light Scattering) shader by Thorsten Renk.
 * Ported to HDR by Fernando García Liñán.
 */

layout(location = 0) out vec4 fragColor;

in VS_OUT {
    float flogz;
    vec3 ls_pos;
    vec3 view_dir;
} fs_in;

uniform float flame_radius_fraction;
uniform float thrust_collimation;
uniform float thrust_density;
uniform float base_flame_density;
uniform float shock_frequency;
uniform float noise_strength;
uniform float noise_scale;
uniform float random_seed;
uniform float deflection_coeff;

uniform bool use_shocks;
uniform bool use_noise;

uniform float flame_brightness;
uniform vec3 flame_color_low;
uniform vec3 flame_color_high;
uniform vec3 base_flame;

uniform float osg_SimulationTime;

const int N_STEPS = 15;

// noise.glsl
float noise_2d(vec2 coord, float wavelength);
// logarithmic_depth.glsl
float logdepth_encode(float z);

float spherical_smoothstep(vec3 pos)
{
    float l = length(vec3(pos.x * 0.5, pos.yz));
    return 10.0 * thrust_density * base_flame_density *
        (1.0 - smoothstep(0.5 * flame_radius_fraction, flame_radius_fraction, l));
}

float thrust_flame(vec3 pos)
{
    pos.z += 8.0 * deflection_coeff;

    float d_rad = length(pos.yz - vec2(0.0, deflection_coeff * pos.x * pos.x));
    float longFade = pos.x * 0.2;

    float density = 1.0 - longFade;
    float radius = flame_radius_fraction + thrust_collimation * 1.0 * pow((pos.x+0.1),0.5);

    if (d_rad > radius) {
        return 0.0;
    }

    float noise = 0.0;
    if (use_noise) {
        noise = noise_2d(vec2(pos.x - osg_SimulationTime * 30.0 + random_seed, d_rad), noise_scale);
    }

    density *= (1.0 - smoothstep(0.125, radius, d_rad))
        * (1.0 - noise_strength + noise_strength* noise);

    if (use_shocks) {
        float shock = sin(pos.x * 10.0 * shock_frequency);
        density += shock * shock * shock * shock * (1.0 - longFade)
            * (1.0 - smoothstep(0.25 * flame_radius_fraction, 0.5 * flame_radius_fraction, d_rad))
            * (1.0 - smoothstep(0.0, 1.0, thrust_collimation)) * (1.0 + 0.5 * base_flame_density);
    }

    return 10.0 * thrust_density *  density / (radius * 5.0);
}

void main()
{
    vec3 view_dir = normalize(fs_in.view_dir);

    float x_E, y_E, z_E;
    if (view_dir.x > 0.0) {x_E = 5.0;} else {x_E = 0.0;}
    if (view_dir.y > 0.0) {y_E = 1.0;} else {y_E = -1.0;}
    if (view_dir.z > 0.0) {z_E = 1.0;} else {z_E = -1.0;}

    float t_x = (x_E - fs_in.ls_pos.x) / view_dir.x;
    float t_y = (y_E - fs_in.ls_pos.y) / view_dir.y;
    float t_z = (z_E - fs_in.ls_pos.z) / view_dir.z;

    float t_min = min(t_x, t_y);
    t_min = min(t_min, t_z);

    float dt = t_min  / float(N_STEPS);

    vec3 pstep = view_dir * dt;
    vec3 pos = fs_in.ls_pos;

    float density1 = 0.0;
    float density2 = 0.0;

    for (int i = 0; i < N_STEPS; ++i) {
        pos = pos + pstep;
        density1 += spherical_smoothstep(pos) * dt;
        density2 += thrust_flame(pos) * dt;
    }

    float density = density1 + density2;
    density = 1.0 - exp(-density);
    density1 = 1.0 - exp(-density1);
    density2 = 1.0 - exp(-density2);

    vec3 color = mix(flame_color_low, flame_color_high, density2);
    color = mix(color, base_flame, density1);

    fragColor = vec4(color * flame_brightness, density);
    gl_FragDepth = logdepth_encode(fs_in.flogz);
}
