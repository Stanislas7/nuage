#pragma once

namespace nuage {
namespace ConfigKeys {

    // Root
    inline constexpr char MODEL[] = "model";
    inline constexpr char JSBSIM[] = "jsbsim";
    inline constexpr char SPAWN[] = "spawn";

    // Model
    inline constexpr char NAME[] = "name";
    inline constexpr char PATH[] = "path";
    inline constexpr char TEXTURE[] = "texture";
    inline constexpr char COLOR[] = "color";
    inline constexpr char SCALE[] = "scale";
    inline constexpr char ROTATION[] = "rotation";
    inline constexpr char OFFSET[] = "offset";

    // JSBSim
    inline constexpr char JSBSIM_MODEL[] = "model";
    inline constexpr char JSBSIM_ROOT[] = "rootPath";
    inline constexpr char JSBSIM_LAT[] = "latitudeDeg";
    inline constexpr char JSBSIM_LON[] = "longitudeDeg";
    inline constexpr char JSBSIM_ROLL_TRIM[] = "rollTrim";

    // Spawn 
    inline constexpr char POSITION[] = "position";
    inline constexpr char AIRSPEED[] = "airspeed";
    inline constexpr char HEADING_DEG[] = "headingDeg";

} // namespace ConfigKeys
} // namespace nuage
