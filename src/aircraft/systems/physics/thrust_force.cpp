#include "thrust_force.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "math/quat.hpp"

namespace nuage {

ThrustForce::ThrustForce(const ThrustConfig& config)
    : m_config(config)
{
}

void ThrustForce::init(PropertyBus* state) {
    m_state = state;
}

void ThrustForce::update(float dt) {
    double thrust = m_state->get(Properties::Engine::THRUST, 0.0);
    float thrustForce = static_cast<float>(thrust * m_config.thrustScale);

    Quat orientation = m_state->getQuat(Properties::Orientation::PREFIX);
    Vec3 forward = orientation.rotate(Vec3(0, 0, 1));
    Vec3 thrustVec = forward * thrustForce;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce = currentForce + thrustVec;
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);

    m_state->setVec3(Properties::Forces::THRUST_PREFIX, thrustVec);
}

}
