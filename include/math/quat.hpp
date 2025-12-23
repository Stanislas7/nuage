#pragma once

#include "vec3.hpp"
#include "mat4.hpp"
#include <cmath>

namespace flightsim {

struct Quat {
    float w = 1, x = 0, y = 0, z = 0;

    static Quat identity() { return Quat{1, 0, 0, 0}; }

    static Quat fromAxisAngle(const Vec3& axis, float angle) {
        float half = angle * 0.5f;
        float s = std::sin(half);
        Vec3 n = axis.normalize();
        return Quat{std::cos(half), n.x * s, n.y * s, n.z * s};
    }

    Quat operator*(const Quat& q) const {
        return Quat{
            w*q.w - x*q.x - y*q.y - z*q.z,
            w*q.x + x*q.w + y*q.z - z*q.y,
            w*q.y - x*q.z + y*q.w + z*q.x,
            w*q.z + x*q.y - y*q.x + z*q.w
        };
    }

    Vec3 rotate(const Vec3& v) const {
        Vec3 u{x, y, z};
        return u * (2.0f * u.dot(v))
             + v * (w*w - u.dot(u))
             + u.cross(v) * (2.0f * w);
    }

    Quat normalized() const {
        float len = std::sqrt(w*w + x*x + y*y + z*z);
        return Quat{w/len, x/len, y/len, z/len};
    }

    Mat4 toMat4() const {
        Mat4 m;
        float xx = x*x, yy = y*y, zz = z*z;
        float xy = x*y, xz = x*z, yz = y*z;
        float wx = w*x, wy = w*y, wz = w*z;

        m.m[0] = 1 - 2*(yy + zz);
        m.m[1] = 2*(xy + wz);
        m.m[2] = 2*(xz - wy);

        m.m[4] = 2*(xy - wz);
        m.m[5] = 1 - 2*(xx + zz);
        m.m[6] = 2*(yz + wx);

        m.m[8] = 2*(xz + wy);
        m.m[9] = 2*(yz - wx);
        m.m[10] = 1 - 2*(xx + yy);

        return m;
    }
};

}
