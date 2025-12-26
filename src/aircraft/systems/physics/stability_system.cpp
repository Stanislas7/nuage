#include "stability_system.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include <algorithm>

namespace nuage {

StabilitySystem::StabilitySystem(const StabilityConfig& config)
    : m_config(config)
{
}

void StabilitySystem::init(PropertyBus* state) {
    m_state = state;
}

void StabilitySystem::update(float dt) {
    (void)dt;
    AerodynamicForceBase::AerodynamicData data = computeAerodynamics(m_state);
    if (data.airSpeed < m_config.minAirspeed) {
        return;
    }
    if (std::abs(data.forwardSpeed) < m_config.minAirspeed) {
        return;
    }

    Vec3 angularVel = m_state->getVec3(Properties::Physics::ANGULAR_VELOCITY_PREFIX);

    constexpr float kMaxAngle = 0.5f;
    float aoa = std::clamp(data.aoa, -kMaxAngle, kMaxAngle);
    float sideslip = std::clamp(data.sideslip, -kMaxAngle, kMaxAngle);

    float speed = std::max(data.airSpeed, m_config.minAirspeed);
    float rateScale = (m_config.referenceLength > 0.0f)
        ? (m_config.referenceLength / (2.0f * speed))
        : 0.0f;

    float qbar = data.dynamicPressure;
    float area = std::max(m_config.referenceArea, 0.01f);
    float length = std::max(m_config.referenceLength, 0.01f);
    float momentScale = m_config.momentScale;

    float pitchMoment = (-m_config.pitchStability * aoa
        - m_config.pitchDamping * angularVel.x * rateScale) * qbar * area * length * momentScale;

    float yawMoment = (-m_config.yawStability * sideslip
        - m_config.yawDamping * angularVel.y * rateScale) * qbar * area * length * momentScale;

    float rollMoment = (-m_config.rollStability * sideslip
        - m_config.rollDamping * angularVel.z * rateScale) * qbar * area * length * momentScale;

    Vec3 currentTorque = m_state->getVec3(Properties::Physics::TORQUE_PREFIX);
    currentTorque.x += pitchMoment;
    currentTorque.y += yawMoment;
    currentTorque.z += rollMoment;
    m_state->setVec3(Properties::Physics::TORQUE_PREFIX, currentTorque);
}

} // namespace nuage
