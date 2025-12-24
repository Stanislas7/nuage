#include "gravity_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"

namespace nuage {

GravitySystem::GravitySystem(const GravityConfig& config)
    : m_config(config)
{
}

void GravitySystem::init(PropertyBus* state) {
    m_state = state;
}

void GravitySystem::update(float dt) {
    double mass = m_state->get(Properties::Physics::MASS, 1000.0);
    double gravityForce = mass * m_config.gravity;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce.y -= static_cast<float>(gravityForce);
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);
}

}
