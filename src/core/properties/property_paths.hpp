#pragma once

#include "core/properties/property_id.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace nuage {
namespace Properties {

namespace Controls {
    inline constexpr TypedProperty<double> ELEVATOR("controls/flight/elevator");
    inline constexpr TypedProperty<double> AILERON("controls/flight/aileron");
    inline constexpr TypedProperty<double> RUDDER("controls/flight/rudder");
    inline constexpr TypedProperty<double> THROTTLE("controls/engines/current/throttle");
    inline constexpr TypedProperty<double> BRAKE_LEFT("controls/gear/brake-left");
    inline constexpr TypedProperty<double> BRAKE_RIGHT("controls/gear/brake-right");
}

namespace Atmosphere {
    inline constexpr TypedProperty<double> DENSITY("atmosphere/density");
    inline constexpr TypedProperty<Vec3> WIND_PREFIX("atmosphere/wind");
}

namespace Velocities {
    inline constexpr TypedProperty<double> AIRSPEED_KT("velocities/airspeed-kt");
    inline constexpr TypedProperty<double> VERTICAL_SPEED_FPS("velocities/vertical-speed-fps");
}

namespace Position {
    inline constexpr TypedProperty<double> ALTITUDE_FT("position/altitude-ft");
    inline constexpr TypedProperty<double> LATITUDE_DEG("position/latitude-deg");
    inline constexpr TypedProperty<double> LONGITUDE_DEG("position/longitude-deg");
}

namespace Orientation {
    inline constexpr TypedProperty<double> PITCH_DEG("orientation/pitch-deg");
    inline constexpr TypedProperty<double> ROLL_DEG("orientation/roll-deg");
    inline constexpr TypedProperty<double> HEADING_DEG("orientation/heading-deg");
}

namespace Sim {
    inline constexpr TypedProperty<bool> PAUSED("sim/paused");
    inline constexpr TypedProperty<bool> QUIT_REQUESTED("sim/quit-requested");
    inline constexpr TypedProperty<double> TIME("sim/time");
    inline constexpr TypedProperty<bool> DEBUG_VISIBLE("sim/debug-visible");
}

} // namespace Properties
} // namespace nuage
