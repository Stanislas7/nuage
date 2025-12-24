#include "lift_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "math/quat.hpp"

namespace nuage {

LiftSystem::LiftSystem(const LiftConfig& config)
    : m_config(config)
{
}

void LiftSystem::init(PropertyBus* state) {
    m_state = state;
}

void LiftSystem::update(float dt) {
    Vec3 velocity = m_state->getVec3(Properties::Velocity::PREFIX);
    double density = m_state->get(Properties::Atmosphere::DENSITY, 1.225);

    float speed = velocity.length();
    if (speed < 1.0f) {
        return;
    }

    Quat orientation = m_state->getQuat(Properties::Orientation::PREFIX);
    Vec3 forward = orientation.rotate(Vec3(0, 0, 1));
    Vec3 up = orientation.rotate(Vec3(0, 1, 0));

    float forwardSpeed = forward.dot(velocity);
    if (forwardSpeed < 0.1f) {
        return;
    }

    float dynamicPressure = 0.5f * static_cast<float>(density) * forwardSpeed * forwardSpeed;
    float liftMagnitude = m_config.liftCoefficient * dynamicPressure * m_config.wingArea;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce = currentForce + up * liftMagnitude;
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);
}

}
