$FG_GLSL_VERSION

// Yaw
void rotation_matrix_H(in float sinRz, in float cosRz, out mat3 rotmat)
{
    rotmat = mat3(  cosRz,  -sinRz, 0.0,
                    sinRz,  cosRz,  0.0,
                    0.0  ,  0.0  ,  1.0);
}

// Pitch
void rotation_matrix_P(in float sinRy, in float cosRy, out mat3 rotmat)
{
    rotmat = mat3(  cosRy,  0.0  , sinRy,
                    0.0  ,  1.0  ,   0.0,
                   -sinRy,  0.0  , cosRy);
}

// Roll
void rotation_matrix_R(in float sinRx, in float cosRx, out mat3 rotmat)
{
    rotmat = mat3(  1.0  ,  0.0  ,    0.0,
                    0.0  , cosRx , -sinRx,
                    0.0  , sinRx  , cosRx);
}

void scale_matrix(in float scale, out mat3 scalemat)
{
    scalemat = mat3(scale, 0.0  , 0.0  ,
                    0.0  , scale, 0.0  ,
                    0.0  , 0.0  , scale);
}

/*
 * Takes the position, normal of an vertex, and position/rotation/scale of
 * one instance, and transforms the original position and normal of the
 * vertex to match it's instanced version.
 */
void apply_instance_transforms(inout vec3 position,
                               inout vec3 normal,
                               in vec3 instance_position,
                               in vec4 instance_rotation_and_scale)
{
    // Handle rotation and scaling
    mat3 ScaleMat;
    scale_matrix(instance_rotation_and_scale.w, ScaleMat);

    mat3 RotMatH;
    mat3 RotMatP;
    mat3 RotMatR;

    float hdg = instance_rotation_and_scale.x;
    float pitch = instance_rotation_and_scale.y;
    float roll = instance_rotation_and_scale.z;
    // if (roll > 90.0 || roll < -90.0) {roll = -roll;}
    float cosRx = cos(radians(-roll));
    float sinRx = sin(radians(-roll));
    float cosRy = cos(radians(-pitch));
    float sinRy = sin(radians(-pitch));
    float cosRz = cos(radians(-hdg));
    float sinRz = sin(radians(-hdg));

    rotation_matrix_R(sinRx, cosRx, RotMatR);
    rotation_matrix_P(sinRy, cosRy, RotMatP);
    rotation_matrix_H(sinRz, cosRz, RotMatH);

    // Transform position using scaling and rotation matrices
    position = RotMatH * RotMatP * RotMatR * ScaleMat * position;

    // Offset model to correct location w.r.t instancing center
    position += instance_position;

    // Transform normal using rotation matrices
    normal = RotMatH * RotMatP * RotMatR * normal;
}
