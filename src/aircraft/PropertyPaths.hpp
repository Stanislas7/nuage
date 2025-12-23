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
}

namespace Engine {
    inline constexpr char THRUST[] = "engine/thrust";
    inline constexpr char N1[] = "engine/n1";
    inline constexpr char RUNNING[] = "engine/running";
    inline constexpr char FUEL_FLOW[] = "engine/fuel_flow";
}

// ============== FUEL ==============
namespace Fuel {
    inline constexpr char QUANTITY[] = "fuel/quantity";
    inline constexpr char CAPACITY[] = "fuel/capacity";
}

// ============== ATMOSPHERE ==============
namespace Atmosphere {
    inline constexpr char DENSITY[] = "atmosphere/density";
    inline constexpr char PRESSURE[] = "atmosphere/pressure";
    inline constexpr char TEMPERATURE[] = "atmosphere/temperature";
    inline constexpr char WIND_SPEED[] = "atmosphere/wind_speed";
    inline constexpr char WIND_HEADING[] = "atmosphere/wind_heading";
}

// ============== LANDING GEAR (Future) ==============
namespace Gear {
    inline constexpr char EXTENSION[] = "gear/extension";
}

// ============== FLAPS (Future) ==============
namespace Flaps {
    inline constexpr char SETTING[] = "flaps/setting";
}

} // namespace Properties
} // namespace nuage
