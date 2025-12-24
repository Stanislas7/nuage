#include "orientation_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "math/quat.hpp"
#include <algorithm>

namespace nuage {

OrientationSystem::OrientationSystem(const OrientationConfig& config)
    : m_config(config)
{
}

void OrientationSystem::init(PropertyBus* state) {
    m_state = state;
}

void OrientationSystem::update(float dt) {
    double pitch = m_state->get(Properties::Input::PITCH);
    double yaw = m_state->get(Properties::Input::YAW);
    double roll = m_state->get(Properties::Input::ROLL);

    Vec3 velocity = m_state->getVec3(Properties::Velocity::PREFIX);
    float speed = velocity.length();
    
    // Scale control authority with speed
    // This approximates dynamic pressure (q = 0.5 * rho * v^2)
    float controlScale = (m_config.controlRefSpeed > 0.0f)
        ? (speed / m_config.controlRefSpeed)
        : 1.0f;
    controlScale = std::clamp(controlScale, m_config.minControlScale, m_config.maxControlScale);
    
    // Multipliers tuned to give reasonable response with the new inertia model
    // TORQUE_MULTIPLIER acts as "Stiffness" (how fast we reach target rate)
    // DAMPING_FACTOR acts as "Resistance" (keeps us from spinning forever)
    const float TORQUE_MULTIPLIER = m_config.torqueMultiplier; 
    const float DAMPING_FACTOR = m_config.dampingFactor; 
    
    // Calculate Aerodynamic Damping
    float damping = DAMPING_FACTOR * controlScale;

    // Torques in Body Frame
    // Standard RHR: +X Pitch Up, +Y Yaw Left, +Z Roll Right
    // But verify input mapping:
    // Pitch Input (+1) -> Nose Up -> +X Torque
    // Yaw Input (+1) -> Nose Right -> -Y Torque
    // Roll Input (+1) -> Bank Right -> +Z Torque (Assuming Z is Forward)
    
    float pitchTorque = static_cast<float>(pitch * m_config.pitchRate * TORQUE_MULTIPLIER * controlScale);
    float yawTorque   = static_cast<float>(-yaw * m_config.yawRate * TORQUE_MULTIPLIER * controlScale);
    float rollTorque  = static_cast<float>(roll * m_config.rollRate * TORQUE_MULTIPLIER * controlScale);

    // Apply Damping
    Vec3 angularVel = m_state->getVec3(Properties::Physics::ANGULAR_VELOCITY_PREFIX);
    
    pitchTorque -= angularVel.x * damping;
    yawTorque   -= angularVel.y * damping;
    rollTorque  -= angularVel.z * damping;

    // Apply to bus
    Vec3 currentTorque = m_state->getVec3(Properties::Physics::TORQUE_PREFIX);
    currentTorque.x += pitchTorque;
    currentTorque.y += yawTorque;
    currentTorque.z += rollTorque;
    
    m_state->setVec3(Properties::Physics::TORQUE_PREFIX, currentTorque);
}

}
