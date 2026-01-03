$FG_GLSL_VERSION
/*
 * Chute deformation shader
 * Original ALS (Atmospheric Light Scattering) shader by Thorsten Renk.
 * Ported to HDR by Fernando García Liñán.
 */

uniform float chute_force;
uniform float chute_fold;
uniform float chute_bend;
uniform float chute_projection_z;

uniform float osg_SimulationTime;

vec3 chute_apply_deformation(vec3 pos)
{
    float force_speedup = max(0.8, chute_force);

    float phi = asin(dot(vec2(1.0, 0.0), normalize(pos.xy)));
    float wobble = 0.02 * sin(12.0 * osg_SimulationTime) * sin(14.0 * phi);
    wobble += 0.01 * sin(6.0 * osg_SimulationTime) * sin(8.0 * phi);

    pos.xy *= (1.0 + wobble * (1.0 - 0.5 * chute_fold));

    float displace_x = pos.z * sin(6.0 * osg_SimulationTime * force_speedup) * 0.05;
    float displace_y = pos.z * cos(4.0 * osg_SimulationTime * force_speedup) * 0.04;

    pos.xy += vec2(displace_x, displace_y) * (1.0 - chute_fold);

    // distort shape with force
    float force_gain;
    float force_factor = abs(chute_force - 1.0);

    if (chute_force < 1.0) {
        force_gain = 1.0 + 0.2 * force_factor;
    } else {
        force_gain = 1.0 - 0.2 * force_factor;
    }

    pos.xy *= force_gain;

    pos.z +=  0.4 * (1.0 / force_gain - 1.0) * max(pos.z - chute_projection_z, 0.0);

    // fold
    pos.z = chute_projection_z + (1.0 - 0.8 * chute_fold) * (pos.z - chute_projection_z);
    pos.z += 0.5 * chute_fold * (sin(2.0 * pos.x + 1.0 * osg_SimulationTime) *
                                 cos(3.0 * pos.y + 1.5 * osg_SimulationTime) * 0.5);
    // and bend
    pos.z -= pos.x * pos.x * 0.2 * chute_bend;

    return pos;
}
