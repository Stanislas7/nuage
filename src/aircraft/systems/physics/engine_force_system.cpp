#include "engine_force_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "math/quat.hpp"

namespace nuage {

EngineForceSystem::EngineForceSystem(const EngineForceConfig& config)
    : m_config(config)
{
}

void EngineForceSystem::init(PropertyBus* state) {
    m_state = state;
}

void EngineForceSystem::update(float dt) {
    double thrust = m_state->get(Properties::Engine::THRUST, 0.0);
    float thrustForce = static_cast<float>(thrust * m_config.thrustScale);

    Quat orientation = m_state->getQuat(Properties::Orientation::PREFIX);
    Vec3 forward = orientation.rotate(Vec3(0, 0, 1));

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce = currentForce + forward * thrustForce;
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);
}

}
