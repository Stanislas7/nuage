#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace nuage {

/**
 * @brief High-performance struct for "hot path" aircraft state.
 * These properties are updated every physics tick and interpolated for rendering.
 */
struct AircraftState {
    Vec3 position = Vec3(0, 0, 0);
    Quat orientation = Quat::identity();
    Vec3 velocity = Vec3(0, 0, 0);        // World-space linear velocity
    Vec3 angularVelocity = Vec3(0, 0, 0); // Body-space angular velocity (p, q, r)
    double airspeed = 0.0;
};

} // namespace nuage