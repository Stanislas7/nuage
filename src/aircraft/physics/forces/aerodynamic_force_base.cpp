#include "aerodynamic_force_base.hpp"
#include "aircraft/physics/physics_constants.hpp"
#include "core/property_paths.hpp"

namespace nuage {

AerodynamicForceBase::AerodynamicData AerodynamicForceBase::computeAerodynamics(PropertyBus* state) const {
    AerodynamicData data;

    data.velocity = state->getVec3(Properties::Velocity::PREFIX);
    data.speed = data.velocity.length();
    data.airflowDir = data.speed > 0.001f ? data.velocity.normalize() : Vec3(0, 0, 1);
    
    data.airDensity = static_cast<float>(state->get(Properties::Atmosphere::DENSITY, PhysicsConstants::SEA_LEVEL_DENSITY));
    data.dynamicPressure = 0.5f * data.airDensity * data.speed * data.speed;

    data.orientation = state->getQuat(Properties::Orientation::PREFIX);
    data.forward = data.orientation.rotate(Vec3(0, 0, 1));
    data.up = data.orientation.rotate(Vec3(0, 1, 0));
    data.right = data.orientation.rotate(Vec3(1, 0, 0));

    return data;
}

}
