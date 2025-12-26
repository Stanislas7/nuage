#pragma once

#include "core/properties/property_id.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace nuage {
namespace Properties {

namespace Input {
    inline constexpr TypedProperty<double> PITCH("input/pitch");
    inline constexpr TypedProperty<double> ROLL("input/roll");
    inline constexpr TypedProperty<double> YAW("input/yaw");
    inline constexpr TypedProperty<double> THROTTLE("input/throttle");
}

namespace Atmosphere {
    inline constexpr TypedProperty<double> DENSITY("atmosphere/density");
    inline constexpr TypedProperty<Vec3> WIND_PREFIX("atmosphere/wind");
}

} // namespace Properties
} // namespace nuage
