#pragma once

#include <cmath>

namespace nuage {

struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }

    Vec3 normalize() const {
        float l = std::sqrt(x * x + y * y + z * z);
        return l > 0 ? Vec3(x / l, y / l, z / l) : Vec3();
    }
    
    Vec3 normalized() const { return normalize(); }

    Vec3 cross(const Vec3& o) const {
        return Vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
    }

    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
};

}
