#include "lift_system.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"

namespace nuage {

LiftSystem::LiftSystem(const LiftConfig& config)
    : m_config(config)
{
}

void LiftSystem::init(PropertyBus* state) {
    m_state = state;
}

void LiftSystem::update(float dt) {
    AerodynamicForceBase::AerodynamicData data = computeAerodynamics(m_state);

    if (data.speed < 1.0f) {
        return;
    }

    float forwardSpeed = data.forward.dot(data.velocity);
    if (forwardSpeed < 0.1f) {
        return;
    }

    float dynamicPressure = 0.5f * data.airDensity * forwardSpeed * forwardSpeed;
    float liftMagnitude = m_config.liftCoefficient * dynamicPressure * m_config.wingArea;

    Vec3 liftVec = data.up * liftMagnitude;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce = currentForce + liftVec;
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);

    m_state->setVec3(Properties::Forces::LIFT_PREFIX, liftVec);
}

}
