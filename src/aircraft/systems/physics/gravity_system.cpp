#include "gravity_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "aircraft/physics/physics_constants.hpp"

namespace nuage {

void GravitySystem::init(PropertyBus* state) {
    m_state = state;
}

void GravitySystem::update(float dt) {
    double mass = m_state->get(Properties::Physics::MASS, 1000.0);
    double gravityForce = mass * PhysicsConstants::GRAVITY;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce.y -= static_cast<float>(gravityForce);
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);

    m_state->setVec3(Properties::Forces::GRAVITY_PREFIX, 0.0f, -static_cast<float>(gravityForce), 0.0f);
}

}
