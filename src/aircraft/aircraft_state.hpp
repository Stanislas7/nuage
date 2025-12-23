#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace flightsim {

// Aircraft state - pure data, no behavior
struct AircraftState {
    Vec3 position{0, 100, 0};
    Quat orientation = Quat::identity();
    Vec3 velocity{0, 0, 20};          // m/s

    float throttle = 0.3f;            // 0 to 1
    float speed = 20.0f;              // m/s (computed from velocity)

    // Control surface positions (-1 to 1)
    float elevator = 0;
    float ailerons = 0;
    float rudder = 0;
};

}
