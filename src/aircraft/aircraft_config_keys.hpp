#pragma once

namespace nuage {
namespace ConfigKeys {

    // Root
    inline constexpr char MODEL[] = "model";
    inline constexpr char PHYSICS[] = "physics";
    inline constexpr char ENGINE[] = "engine";
    inline constexpr char FUEL[] = "fuel";
    inline constexpr char ORIENTATION[] = "orientation";
    inline constexpr char ENGINE_FORCE[] = "engineForce";
    inline constexpr char LIFT[] = "lift";
    inline constexpr char DRAG[] = "drag";
    inline constexpr char SPAWN[] = "spawn";

    // Model
    inline constexpr char NAME[] = "name";
    inline constexpr char PATH[] = "path";
    inline constexpr char TEXTURE[] = "texture";
    inline constexpr char COLOR[] = "color";
    inline constexpr char SCALE[] = "scale";
    inline constexpr char ROTATION[] = "rotation";
    inline constexpr char OFFSET[] = "offset";

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
    inline constexpr char FUEL_FLOW_IDLE[] = "fuelFlowIdle";
    inline constexpr char FUEL_FLOW_MAX[] = "fuelFlowMax";

    // Fuel
    inline constexpr char CAPACITY[] = "capacity";
    inline constexpr char INITIAL[] = "initial";

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

    // Lift
    inline constexpr char CL0[] = "cl0";
    inline constexpr char CL_ALPHA[] = "clAlpha";
    inline constexpr char CL_MAX[] = "clMax";
    inline constexpr char CL_MIN[] = "clMin";
    inline constexpr char WING_AREA[] = "wingArea";

    // Drag
    inline constexpr char CD0[] = "cd0";
    inline constexpr char INDUCED_DRAG_FACTOR[] = "inducedDragFactor";
    inline constexpr char FRONTAL_AREA[] = "frontalArea";

    // Spawn 
    inline constexpr char POSITION[] = "position";
    inline constexpr char VELOCITY[] = "velocity";
    inline constexpr char AIRSPEED[] = "airspeed";

} // namespace ConfigKeys
} // namespace nuage
