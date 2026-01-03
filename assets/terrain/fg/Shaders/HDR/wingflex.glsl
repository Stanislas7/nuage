$FG_GLSL_VERSION
/*
 * Wingflex shader
 * Original non-organic deformation by Bea Wolf, organic deformation by Thorsten Renk.
 * Ported to HDR by Fernando García Liñán.
 */

uniform int wingflex_type;
uniform float body_width;
uniform float wing_span;
uniform float wingflex_alpha;
uniform float wingflex_trailing_alpha;
uniform float wingsweep_factor;
uniform float wingflex_z;
uniform float rotation_rad;
uniform vec3 rotation_1;
uniform vec3 rotation_2;

vec2 calc_deflection(float y)
{
    float dist;
    float bwh = body_width * 0.5;
    if (y < bwh && y > -bwh) {
        // this part does not move
        dist = 0;
    } else if (y > bwh) {
        dist = y - bwh;
    } else if (y < -bwh) {
        dist = y + bwh;
    }
    float max_dist = (wing_span - body_width) * 0.5;
    float deflection = wingflex_z * (dist*dist) / (max_dist*max_dist);
    float delta_y;
    if (y < 0) {
        delta_y = deflection / wing_span;
    } else {
        delta_y = -deflection / wing_span;
    }
    vec2 returned = vec2(deflection, delta_y);
    return returned;
}

vec3 wingflex_apply_deformation(vec3 pos)
{
    if (wingflex_type == 0) { // Non-organic
        vec2 deflection = calc_deflection(pos.y);

        pos.z += deflection[0];
        pos.y += deflection[1];

        if (rotation_rad != 0.0) {
            vec2 defl1=calc_deflection(rotation_1.y);
            vec2 defl2=calc_deflection(rotation_2.y);
            float rot_y1 = rotation_1.y;
            float rot_z1 = rotation_1.z;
            float rot_y2 = rotation_2.y;
            float rot_z2 = rotation_2.z;
            rot_y1 -= defl1[1];
            rot_z1 += defl1[0];
            rot_y2 -= defl2[1];
            rot_z2 += defl2[0];
            // Calculate rotation
            vec3 normal;
            normal[0] = rotation_2.x - rotation_1.x;
            normal[1] = rot_y2 - rot_y1;
            normal[2] = rot_z2 - rot_z1;
            normal = normalize(normal);
            float tmp = (1.0 - cos(rotation_rad));
            mat4 rotation_matrix = mat4(
                pow(normal[0],2)*tmp+cos(rotation_rad),              normal[1]*normal[0]*tmp-normal[2]*sin(rotation_rad), normal[2]*normal[0]*tmp+normal[1]*sin(rotation_rad), 0.0,
                normal[0]*normal[1]*tmp+normal[2]*sin(rotation_rad), pow(normal[1],2)*tmp+cos(rotation_rad),              normal[2]*normal[1]*tmp-normal[0]*sin(rotation_rad), 0.0,
                normal[0]*normal[2]*tmp-normal[1]*sin(rotation_rad), normal[1]*normal[2]*tmp+normal[0]*sin(rotation_rad), pow(normal[2],2)*tmp+cos(rotation_rad),               0.0,
                0.0,                                                 0.0,                                                 0.0,                                                 1.0);
            vec4 old_point;
            old_point[0] = pos.x;
            old_point[1] = pos.y;
            old_point[2] = pos.z;
            old_point[3] = 1.0;
            rotation_matrix[3][0] = rotation_1.x - rotation_1.x*rotation_matrix[0][0] - rot_y1*rotation_matrix[1][0] - rot_z1*rotation_matrix[2][0];
            rotation_matrix[3][1] = rot_y1      - rotation_1.x*rotation_matrix[0][1] - rot_y1*rotation_matrix[1][1] - rot_z1*rotation_matrix[2][1];
            rotation_matrix[3][2] = rot_z1      - rotation_1.x*rotation_matrix[0][2] - rot_y1*rotation_matrix[1][2] - rot_z1*rotation_matrix[2][2];
            vec4 new_point = rotation_matrix * old_point;
            pos.x = new_point[0];
            pos.y = new_point[1];
            pos.z = new_point[2];
        }
    } else if (wingflex_type == 1) { // Organic
        const float arm_reach = 4.8;

        float x_factor = max((abs(pos.x) - body_width), 0.0);
        float y_factor = max(pos.y, 0.0);
        float flex_factor1 = wingflex_alpha * (1.0 - wingsweep_factor);
        float flex_factor2 = wingflex_trailing_alpha * (1.0 - wingsweep_factor);

        if (flex_factor1 < 0.0) {
            flex_factor1 *= 0.7;
        }
        if (flex_factor2 < 0.0) {
            flex_factor1 *= 0.7;
        }

        // basic flapping motion is linear to arm_reach, then parabolic
        float intercept_point = 0.1 * arm_reach * arm_reach * flex_factor1;

        if (x_factor < arm_reach) {
            pos.z += x_factor / arm_reach * intercept_point;
        } else {
            pos.z += 0.1 * x_factor * x_factor * flex_factor1;
        }

        // upward stroke is slightly forward-swept, downward stroke a bit backward
        pos.y += -0.25 * abs(x_factor) * flex_factor1;

        //trailing edge lags the motion
        pos.z += 0.2 * y_factor * x_factor * flex_factor2;

        // if the wings are folded, we sweep them back
        pos.y += 0.5 * x_factor * wingsweep_factor;

        float sweep_x = 0.5;
        if (pos.x > 0.0) {
            sweep_x = -0.5;
        }
        pos.x += sweep_x * (1.0 + 0.5 * x_factor) * wingsweep_factor;
    }
    return pos;
}
