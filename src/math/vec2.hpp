#pragma once

#include <cmath>

namespace nuage {

struct Vec2 {
    float x, y;

    Vec2(float x = 0, float y = 0) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    
    float length() const {
        return std::sqrt(x * x + y * y);
    }
};

}
