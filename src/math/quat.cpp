#include "math/quat.hpp"
#include "math/mat4.hpp"

namespace nuage {

Mat4 Quat::toMat4() const {
    Mat4 m;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    m.m[0] = 1 - 2 * (yy + zz);
    m.m[1] = 2 * (xy + wz);
    m.m[2] = 2 * (xz - wy);

    m.m[4] = 2 * (xy - wz);
    m.m[5] = 1 - 2 * (xx + zz);
    m.m[6] = 2 * (yz + wx);

    m.m[8] = 2 * (xz + wy);
    m.m[9] = 2 * (yz - wx);
    m.m[10] = 1 - 2 * (xx + yy);

    return m;
}

}
