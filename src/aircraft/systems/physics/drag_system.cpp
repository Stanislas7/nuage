#include "drag_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"

namespace nuage {

DragSystem::DragSystem(const DragConfig& config)
    : m_config(config)
{
}

void DragSystem::init(PropertyBus* state) {
    m_state = state;
}

void DragSystem::update(float dt) {
    Vec3 velocity = m_state->getVec3(Properties::Velocity::PREFIX);
    double density = m_state->get(Properties::Atmosphere::DENSITY, 1.225);

    float speed = velocity.length();
    if (speed < 0.01f) {
        return;
    }

    Vec3 dragDir = -velocity.normalize();

    float dynamicPressure = 0.5f * static_cast<float>(density) * speed * speed;
    float dragMagnitude = m_config.dragCoefficient * dynamicPressure * m_config.frontalArea;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce = currentForce + dragDir * dragMagnitude;
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);
}

}
