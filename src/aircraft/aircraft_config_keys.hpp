#pragma once

namespace nuage {
namespace ConfigKeys {

    // Root
    inline constexpr char MODEL[] = "model";
    inline constexpr char PHYSICS[] = "physics";
    inline constexpr char ENGINE[] = "engine";
    inline constexpr char ORIENTATION[] = "orientation";
    inline constexpr char JSBSIM[] = "jsbsim";
    inline constexpr char ENGINE_FORCE[] = "engineForce";
    inline constexpr char LIFT[] = "lift";
    inline constexpr char DRAG[] = "drag";
    inline constexpr char STABILITY[] = "stability";
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
    inline constexpr char JSBSIM_ENABLED[] = "enabled";
    inline constexpr char JSBSIM_MODEL[] = "model";
    inline constexpr char JSBSIM_ROOT[] = "rootPath";
    inline constexpr char JSBSIM_LAT[] = "latitudeDeg";
    inline constexpr char JSBSIM_LON[] = "longitudeDeg";

    // Physics
    inline constexpr char MASS[] = "mass";
    inline constexpr char INERTIA[] = "inertia";
    inline constexpr char CRUISE_SPEED[] = "cruiseSpeed";
    inline constexpr char MIN_ALTITUDE[] = "minAltitude";
    inline constexpr char MAX_CLIMB_RATE[] = "maxClimbRate";
    inline constexpr char GROUND_FRICTION[] = "groundFriction";

    // Engine
    inline constexpr char MAX_THRUST[] = "maxThrust";
    inline constexpr char MAX_POWER_KW[] = "maxPowerKw";
    inline constexpr char IDLE_N1[] = "idleN1";
    inline constexpr char MAX_N1[] = "maxN1";
    inline constexpr char SPOOL_RATE[] = "spoolRate";

    // Orientation
    inline constexpr char PITCH_RATE[] = "pitchRate";
    inline constexpr char YAW_RATE[] = "yawRate";
    inline constexpr char ROLL_RATE[] = "rollRate";
    inline constexpr char CONTROL_REF_SPEED[] = "controlRefSpeed";
    inline constexpr char MIN_CONTROL_SCALE[] = "minControlScale";
    inline constexpr char MAX_CONTROL_SCALE[] = "maxControlScale";
    inline constexpr char TORQUE_MULTIPLIER[] = "torqueMultiplier";
    inline constexpr char DAMPING_FACTOR[] = "dampingFactor";

    // Thrust Force
    inline constexpr char THRUST_SCALE[] = "thrustScale";
    inline constexpr char PROP_EFFICIENCY[] = "propEfficiency";
    inline constexpr char MIN_AIRSPEED[] = "minAirspeed";
    inline constexpr char MAX_STATIC_THRUST[] = "maxStaticThrust";

    // Lift
    inline constexpr char CL0[] = "cl0";
    inline constexpr char CL_ALPHA[] = "clAlpha";
    inline constexpr char CL_MAX[] = "clMax";
    inline constexpr char CL_MIN[] = "clMin";
    inline constexpr char STALL_ALPHA_DEG[] = "stallAlphaDeg";
    inline constexpr char POST_STALL_ALPHA_DEG[] = "postStallAlphaDeg";
    inline constexpr char CL_POST_STALL[] = "clPostStall";
    inline constexpr char CL_POST_STALL_NEG[] = "clPostStallNeg";
    inline constexpr char WING_AREA[] = "wingArea";

    // Drag
    inline constexpr char CD0[] = "cd0";
    inline constexpr char CD_STALL[] = "cdStall";
    inline constexpr char INDUCED_DRAG_FACTOR[] = "inducedDragFactor";
    inline constexpr char FRONTAL_AREA[] = "frontalArea";

    // Stability
    inline constexpr char PITCH_STABILITY[] = "pitchStability";
    inline constexpr char YAW_STABILITY[] = "yawStability";
    inline constexpr char ROLL_STABILITY[] = "rollStability";
    inline constexpr char PITCH_DAMPING[] = "pitchDamping";
    inline constexpr char YAW_DAMPING[] = "yawDamping";
    inline constexpr char ROLL_DAMPING[] = "rollDamping";
    inline constexpr char REFERENCE_AREA[] = "referenceArea";
    inline constexpr char REFERENCE_LENGTH[] = "referenceLength";
    inline constexpr char MOMENT_SCALE[] = "momentScale";
    inline constexpr char STABILITY_MIN_AIRSPEED[] = "minAirspeed";

    // Spawn 
    inline constexpr char POSITION[] = "position";
    inline constexpr char VELOCITY[] = "velocity";
    inline constexpr char AIRSPEED[] = "airspeed";

} // namespace ConfigKeys
} // namespace nuage
