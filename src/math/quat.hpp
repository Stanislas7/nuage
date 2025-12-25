#pragma once

#include "vec3.hpp"
#include "mat4.hpp"
#include <cmath>

namespace nuage {

struct Quat {
    float w = 1, x = 0, y = 0, z = 0;

    Quat() = default;
    Quat(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}
    Quat(double w_, double x_, double y_, double z_) 
        : w(static_cast<float>(w_)), x(static_cast<float>(x_))
        , y(static_cast<float>(y_)), z(static_cast<float>(z_)) {}

    static Quat identity() { return Quat(1.0f, 0.0f, 0.0f, 0.0f); }

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

    static Quat slerp(const Quat& a, const Quat& b, float t) {
        float cosHalfTheta = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
        
        Quat end = b;
        if (cosHalfTheta < 0.0f) {
            end.w = -b.w; end.x = -b.x; end.y = -b.y; end.z = -b.z;
            cosHalfTheta = -cosHalfTheta;
        }

        if (std::abs(cosHalfTheta) >= 1.0f) {
            return a;
        }

        float halfTheta = std::acos(cosHalfTheta);
        float sinHalfTheta = std::sqrt(1.0f - cosHalfTheta*cosHalfTheta);

        if (std::abs(sinHalfTheta) < 0.001f) {
            // Linear interpolation for very small angles
            return Quat(
                a.w * 0.5f + end.w * 0.5f,
                a.x * 0.5f + end.x * 0.5f,
                a.y * 0.5f + end.y * 0.5f,
                a.z * 0.5f + end.z * 0.5f
            ).normalized();
        }

        float ratioA = std::sin((1.0f - t) * halfTheta) / sinHalfTheta;
        float ratioB = std::sin(t * halfTheta) / sinHalfTheta;

        return Quat(
            a.w * ratioA + end.w * ratioB,
            a.x * ratioA + end.x * ratioB,
            a.y * ratioA + end.y * ratioB,
            a.z * ratioA + end.z * ratioB
        );
    }
};

}
