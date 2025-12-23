#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "math/mat4.hpp"

namespace flightsim {

struct Transform {
    Vec3 position{0, 0, 0};
    Quat rotation = Quat::identity();
    Vec3 scale{1, 1, 1};

    Mat4 matrix() const {
        return Mat4::translate(position)
             * rotation.toMat4()
             * Mat4::scale(scale.x, scale.y, scale.z);
    }

    Vec3 forward() const { return rotation.rotate(Vec3{0, 0, 1}); }
    Vec3 up() const { return rotation.rotate(Vec3{0, 1, 0}); }
    Vec3 right() const { return rotation.rotate(Vec3{1, 0, 0}); }
};

}
