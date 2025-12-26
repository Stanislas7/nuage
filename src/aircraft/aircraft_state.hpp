#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace nuage {

/**
 * @brief High-performance struct for "hot path" aircraft state.
 * Used for rendering interpolation and frequent accessors to avoid
 * PropertyBus map lookups.
 */
struct AircraftState {
    Vec3 position = Vec3(0, 0, 0);
    Quat orientation = Quat::identity();
    Vec3 velocity = Vec3(0, 0, 0);
    double airspeed = 0.0;
};

} // namespace nuage
