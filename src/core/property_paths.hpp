#pragma once

#include "core/property_id.hpp"
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

namespace Position {
    inline constexpr TypedProperty<Vec3> PREFIX("position");
}

namespace Orientation {
    inline constexpr TypedProperty<Quat> PREFIX("orientation");
}

namespace Velocity {
    inline constexpr TypedProperty<double> AIRSPEED("velocity/airspeed");
    inline constexpr TypedProperty<double> VERTICAL("velocity/vertical");
    inline constexpr TypedProperty<Vec3> PREFIX("velocity");
}

namespace Physics {
    inline constexpr TypedProperty<double> MASS("physics/mass");
    inline constexpr TypedProperty<Vec3> FORCE_PREFIX("physics/force");
    inline constexpr TypedProperty<Vec3> ACCEL_PREFIX("physics/acceleration");
    inline constexpr TypedProperty<double> AIR_SPEED("physics/air_speed");
    inline constexpr TypedProperty<Vec3> INERTIA("physics/inertia");
    inline constexpr TypedProperty<Vec3> TORQUE_PREFIX("physics/torque");
    inline constexpr TypedProperty<Vec3> ANGULAR_VELOCITY_PREFIX("physics/angular_velocity");
    inline constexpr TypedProperty<Vec3> ANGULAR_ACCEL_PREFIX("physics/angular_acceleration");
}

namespace Forces {
    inline constexpr TypedProperty<Vec3> TOTAL_PREFIX("forces/total");
    inline constexpr TypedProperty<Vec3> GRAVITY_PREFIX("forces/gravity");
    inline constexpr TypedProperty<Vec3> LIFT_PREFIX("forces/lift");
    inline constexpr TypedProperty<Vec3> DRAG_PREFIX("forces/drag");
    inline constexpr TypedProperty<Vec3> THRUST_PREFIX("forces/thrust");
}

namespace Engine {
    inline constexpr TypedProperty<double> THRUST("engine/thrust");
    inline constexpr TypedProperty<double> N1("engine/n1");
    inline constexpr TypedProperty<bool> RUNNING("engine/running");
    inline constexpr TypedProperty<double> POWER("engine/power");
}

namespace Atmosphere {
    inline constexpr TypedProperty<double> DENSITY("atmosphere/density");
    inline constexpr TypedProperty<double> PRESSURE("atmosphere/pressure");
    inline constexpr TypedProperty<double> TEMPERATURE("atmosphere/temperature");
    inline constexpr TypedProperty<double> WIND_SPEED("atmosphere/wind_speed");
    inline constexpr TypedProperty<double> WIND_HEADING("atmosphere/wind_heading");
    inline constexpr TypedProperty<Vec3> WIND_PREFIX("atmosphere/wind");
}

namespace Gear {
    inline constexpr TypedProperty<double> EXTENSION("gear/extension");
}

namespace Flaps {
    inline constexpr TypedProperty<double> SETTING("flaps/setting");
}

namespace Aero {
    inline constexpr TypedProperty<double> AOA("aero/aoa");
    inline constexpr TypedProperty<double> SIDESLIP("aero/sideslip");
    inline constexpr TypedProperty<double> CL("aero/cl");
}

} // namespace Properties
} // namespace nuage