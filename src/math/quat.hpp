#pragma once

#include "math/vec3.hpp"
#include "utils/json.hpp"
#include <cmath>

namespace nuage {

struct Mat4;

struct Quat {
    float w, x, y, z;

    // JSON conversion from Euler angles [x, y, z] in degrees
    friend void from_json(const nlohmann::json& j, Quat& q) {
        if (j.is_array() && j.size() == 3) {
            float rx = j[0].get<float>() * (3.14159265f / 180.0f);
            float ry = j[1].get<float>() * (3.14159265f / 180.0f);
            float rz = j[2].get<float>() * (3.14159265f / 180.0f);
            Quat qx = Quat::fromAxisAngle(Vec3(1, 0, 0), rx);
            Quat qy = Quat::fromAxisAngle(Vec3(0, 1, 0), ry);
            Quat qz = Quat::fromAxisAngle(Vec3(0, 0, 1), rz);
            q = (qz * qy * qx).normalized();
        }
    }

    Quat(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}
    Quat(double w_, double x_, double y_, double z_) 
        : w(static_cast<float>(w_)), x(static_cast<float>(x_))
        , y(static_cast<float>(y_)), z(static_cast<float>(z_)) {}

    static Quat identity() { return Quat(1.0f, 0.0f, 0.0f, 0.0f); }

    static Quat fromAxisAngle(const Vec3& axis, float angle) {
        float half = angle * 0.5f;
        float s = std::sin(half);
        Vec3 n = axis.normalized();
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

    Mat4 toMat4() const;

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