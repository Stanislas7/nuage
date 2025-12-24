#include "aerodynamic_force_base.hpp"
#include "aircraft/physics/physics_constants.hpp"
#include "core/property_paths.hpp"
#include <cmath>

namespace nuage {

AerodynamicForceBase::AerodynamicData AerodynamicForceBase::computeAerodynamics(PropertyBus* state) const {
    AerodynamicData data;

    data.velocity = state->getVec3(Properties::Velocity::PREFIX);
    data.speed = data.velocity.length();

    Vec3 wind = state->getVec3(Properties::Atmosphere::WIND_PREFIX);
    data.airVelocity = data.velocity - wind;
    data.airSpeed = data.airVelocity.length();
    data.airflowDir = data.airSpeed > 0.001f ? data.airVelocity.normalize() : Vec3(0, 0, 1);
    
    data.airDensity = static_cast<float>(state->get(Properties::Atmosphere::DENSITY, PhysicsConstants::SEA_LEVEL_DENSITY));
    data.dynamicPressure = 0.5f * data.airDensity * data.airSpeed * data.airSpeed;

    data.orientation = state->getQuat(Properties::Orientation::PREFIX);
    data.forward = data.orientation.rotate(Vec3(0, 0, 1));
    data.up = data.orientation.rotate(Vec3(0, 1, 0));
    data.right = data.orientation.rotate(Vec3(1, 0, 0));

    data.forwardSpeed = data.forward.dot(data.airVelocity);
    data.rightSpeed = data.right.dot(data.airVelocity);
    data.upSpeed = data.up.dot(data.airVelocity);

    if (data.airSpeed > 0.001f) {
        data.aoa = std::atan2(-data.upSpeed, data.forwardSpeed);
        data.sideslip = std::atan2(data.rightSpeed, data.forwardSpeed);
    } else {
        data.aoa = 0.0f;
        data.sideslip = 0.0f;
    }

    return data;
}

}
