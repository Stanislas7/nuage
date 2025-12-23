#pragma once

#include "vec3.hpp"
#include <cmath>

namespace nuage {

struct Mat4 {
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    static Mat4 identity() { return Mat4(); }

    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 r;
        float f = 1.0f / std::tan(fov * 0.5f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (far + near) / (near - far);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * far * near) / (near - far);
        r.m[15] = 0.0f;
        return r;
    }

    static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
        Vec3 f = (center - eye).normalize();
        Vec3 s = f.cross(up).normalize();
        Vec3 u = s.cross(f);
        Mat4 r;
        r.m[0] = s.x;  r.m[4] = s.y;  r.m[8] = s.z;
        r.m[1] = u.x;  r.m[5] = u.y;  r.m[9] = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -s.dot(eye);
        r.m[13] = -u.dot(eye);
        r.m[14] = f.dot(eye);
        return r;
    }

    static Mat4 translate(float x, float y, float z) {
        Mat4 r;
        r.m[12] = x;
        r.m[13] = y;
        r.m[14] = z;
        return r;
    }

    static Mat4 translate(const Vec3& v) {
        return translate(v.x, v.y, v.z);
    }

    static Mat4 rotateY(float a) {
        Mat4 r;
        r.m[0] = std::cos(a);
        r.m[8] = std::sin(a);
        r.m[2] = -std::sin(a);
        r.m[10] = std::cos(a);
        return r;
    }

    static Mat4 rotateX(float a) {
        Mat4 r;
        r.m[5] = std::cos(a);
        r.m[9] = -std::sin(a);
        r.m[6] = std::sin(a);
        r.m[10] = std::cos(a);
        return r;
    }

    static Mat4 scale(float x, float y, float z) {
        Mat4 r;
        r.m[0] = x;
        r.m[5] = y;
        r.m[10] = z;
        return r;
    }

    static Mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
        Mat4 r;
        r.m[0] = 2.0f / (right - left);
        r.m[5] = 2.0f / (top - bottom);
        r.m[10] = -2.0f / (far - near);
        r.m[12] = -(right + left) / (right - left);
        r.m[13] = -(top + bottom) / (top - bottom);
        r.m[14] = -(far + near) / (far - near);
        return r;
    }

    const float* data() const { return m; }
    float* data() { return m; }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                r.m[j * 4 + i] = 0;
                for (int k = 0; k < 4; k++) {
                    r.m[j * 4 + i] += m[k * 4 + i] * o.m[j * 4 + k];
                }
            }
        }
        return r;
    }
};

}
