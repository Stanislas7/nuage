#pragma once

#include "math/mat4.hpp"
#include "math/vec3.hpp"

namespace nuage {

// Passed to anything that needs to render
struct RenderContext {
    Mat4 view;
    Mat4 projection;
    Mat4 viewProjection;
    Vec3 cameraPosition;

    // Future: lighting, time, etc.
    Vec3 sunDirection{0.5f, -1.0f, 0.3f};
    float time = 0;
};

}
