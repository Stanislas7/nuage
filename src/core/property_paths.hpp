#pragma once

namespace nuage {
namespace Properties {

namespace Input {
    inline constexpr char PITCH[] = "input/pitch";
    inline constexpr char ROLL[] = "input/roll";
    inline constexpr char YAW[] = "input/yaw";
    inline constexpr char THROTTLE[] = "input/throttle";
}

namespace Position {
    inline constexpr char PREFIX[] = "position";
    inline constexpr char X[] = "position/x";
    inline constexpr char Y[] = "position/y";
    inline constexpr char Z[] = "position/z";
}

namespace Orientation {
    inline constexpr char PREFIX[] = "orientation";
    inline constexpr char W[] = "orientation/w";
    inline constexpr char X[] = "orientation/x";
    inline constexpr char Y[] = "orientation/y";
    inline constexpr char Z[] = "orientation/z";
}

namespace Velocity {
    inline constexpr char AIRSPEED[] = "velocity/airspeed";
    inline constexpr char VERTICAL[] = "velocity/vertical";
    inline constexpr char PREFIX[] = "velocity";
    inline constexpr char X[] = "velocity/x";
    inline constexpr char Y[] = "velocity/y";
    inline constexpr char Z[] = "velocity/z";
}

namespace Physics {
    inline constexpr char MASS[] = "physics/mass";
    inline constexpr char FORCE_PREFIX[] = "physics/force";
    inline constexpr char FORCE_X[] = "physics/force/x";
    inline constexpr char FORCE_Y[] = "physics/force/y";
    inline constexpr char FORCE_Z[] = "physics/force/z";
    inline constexpr char ACCEL_PREFIX[] = "physics/acceleration";
    inline constexpr char ACCEL_X[] = "physics/acceleration/x";
    inline constexpr char ACCEL_Y[] = "physics/acceleration/y";
    inline constexpr char ACCEL_Z[] = "physics/acceleration/z";
    inline constexpr char AIR_SPEED[] = "physics/air_speed";
}

namespace Forces {
    inline constexpr char TOTAL_PREFIX[] = "forces/total";
    inline constexpr char GRAVITY_PREFIX[] = "forces/gravity";
    inline constexpr char LIFT_PREFIX[] = "forces/lift";
    inline constexpr char DRAG_PREFIX[] = "forces/drag";
    inline constexpr char THRUST_PREFIX[] = "forces/thrust";
}

namespace Engine {
    inline constexpr char THRUST[] = "engine/thrust";
    inline constexpr char N1[] = "engine/n1";
    inline constexpr char RUNNING[] = "engine/running";
    inline constexpr char FUEL_FLOW[] = "engine/fuel_flow";
    inline constexpr char POWER[] = "engine/power";
}

namespace Fuel {
    inline constexpr char QUANTITY[] = "fuel/quantity";
    inline constexpr char CAPACITY[] = "fuel/capacity";
}

namespace Atmosphere {
    inline constexpr char DENSITY[] = "atmosphere/density";
    inline constexpr char PRESSURE[] = "atmosphere/pressure";
    inline constexpr char TEMPERATURE[] = "atmosphere/temperature";
    inline constexpr char WIND_SPEED[] = "atmosphere/wind_speed";
    inline constexpr char WIND_HEADING[] = "atmosphere/wind_heading";
    inline constexpr char WIND_PREFIX[] = "atmosphere/wind";
    inline constexpr char WIND_X[] = "atmosphere/wind/x";
    inline constexpr char WIND_Y[] = "atmosphere/wind/y";
    inline constexpr char WIND_Z[] = "atmosphere/wind/z";
}

namespace Gear {
    inline constexpr char EXTENSION[] = "gear/extension";
}

namespace Flaps {
    inline constexpr char SETTING[] = "flaps/setting";
}

namespace Aero {
    inline constexpr char AOA[] = "aero/aoa";
    inline constexpr char SIDESLIP[] = "aero/sideslip";
    inline constexpr char CL[] = "aero/cl";
}

} // namespace Properties
} // namespace nuage
