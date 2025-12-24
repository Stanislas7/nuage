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
    
    // We treat 'Rate' config as 'Torque Authority'
    // Multipliers tuned to give reasonable response with the new inertia model
    const float TORQUE_MULTIPLIER = 2000.0f; 
    
    // Torques in Body Frame
    // X: Pitch (Right Axis)
    // Y: Yaw (Up Axis)
    // Z: Roll (Forward Axis)
   
    
    float pitchTorque = static_cast<float>(pitch * m_config.pitchRate * TORQUE_MULTIPLIER * controlScale);
    float yawTorque   = static_cast<float>(-yaw * m_config.yawRate * TORQUE_MULTIPLIER * controlScale); // Inverted for Yaw Right
    float rollTorque  = static_cast<float>(-roll * m_config.rollRate * TORQUE_MULTIPLIER * controlScale);
    
    rollTorque = static_cast<float>(roll * m_config.rollRate * TORQUE_MULTIPLIER * controlScale);


    // Add Aerodynamic Damping (Stability)
    // Resists rotation
    Vec3 angularVel = m_state->getVec3(Properties::Physics::ANGULAR_VELOCITY_PREFIX);
    const float DAMPING_FACTOR = 400.0f; 
    
    // Damping increases with airspeed too (aerodynamic damping)
    float damping = DAMPING_FACTOR * controlScale;
    
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