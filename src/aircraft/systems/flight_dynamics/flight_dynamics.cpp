#include "flight_dynamics.hpp"
#include "aircraft/property_bus.hpp"
#include "aircraft/property_paths.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include <algorithm>
#include <cmath>

namespace nuage {

FlightDynamics::FlightDynamics(const FlightDynamicsConfig& config)
    : m_config(config)
{
}

void FlightDynamics::init(PropertyBus* state) {
    m_state = state;
}

void FlightDynamics::update(float dt) {
    updateThrottle(dt);
    updateOrientation(dt);
    updatePosition(dt);
    enforceConstraints();
}

void FlightDynamics::updateThrottle(float dt) {
    double throttleInput = m_state->get(Properties::Input::THROTTLE);
    double currentSpeed = m_state->get(Properties::Velocity::AIRSPEED, m_config.minSpeed);

    double targetSpeed = m_config.minSpeed +
        (m_config.maxSpeed - m_config.minSpeed) * throttleInput;

    double speedDelta = (targetSpeed - currentSpeed) * m_config.throttleResponse * dt;
    m_state->set(Properties::Velocity::AIRSPEED, currentSpeed + speedDelta);
}

void FlightDynamics::updateOrientation(float dt) {
    double pitch = m_state->get(Properties::Input::PITCH);
    double yaw = m_state->get(Properties::Input::YAW);
    double roll = m_state->get(Properties::Input::ROLL);

    Quat orientation = m_state->getQuat(Properties::Orientation::PREFIX);

    Vec3 fwd = orientation.rotate(Vec3(0, 0, 1));
    Vec3 rgt = orientation.rotate(Vec3(1, 0, 0));

    float pitchDelta = static_cast<float>(pitch * m_config.pitchRate * dt);
    float yawDelta = static_cast<float>(yaw * m_config.yawRate * dt);
    float rollDelta = static_cast<float>(roll * m_config.rollRate * dt);

    Quat pitchRot = Quat::fromAxisAngle(rgt, pitchDelta);
    Quat yawRot = Quat::fromAxisAngle(Vec3(0, 1, 0), yawDelta);
    Quat rollRot = Quat::fromAxisAngle(fwd, rollDelta);

    Quat newOrientation = (yawRot * pitchRot * rollRot * orientation).normalized();

    m_state->setQuat(Properties::Orientation::PREFIX, newOrientation);
}

void FlightDynamics::updatePosition(float dt) {
    Quat orientation = m_state->getQuat(Properties::Orientation::PREFIX);
    Vec3 forward = orientation.rotate(Vec3(0, 0, 1));
    double speed = m_state->get(Properties::Velocity::AIRSPEED);

    float pitchAngle = std::asin(std::clamp(forward.y, -1.0f, 1.0f));
    
    double speedRatio = speed / m_config.cruiseSpeed;
    double liftFactor = std::min(speedRatio * speedRatio, 1.5);
    
    double netVerticalAccel = (liftFactor - 1.0) * m_config.gravity;
    
    double climbRate = speed * std::sin(pitchAngle) + netVerticalAccel * 0.5;
    
    climbRate = std::clamp(climbRate, -20.0, 20.0);

    Vec3 pos = m_state->getVec3(Properties::Position::PREFIX);
    Vec3 horizontalForward = Vec3(forward.x, 0.0f, forward.z);
    float horizLen = std::sqrt(horizontalForward.x * horizontalForward.x + horizontalForward.z * horizontalForward.z);
    if (horizLen > 0.001f) {
        horizontalForward = horizontalForward * (1.0f / horizLen);
    }
    pos = pos + horizontalForward * static_cast<float>(speed * std::cos(pitchAngle) * dt);
    pos.y += static_cast<float>(climbRate * dt);
    
    m_state->setVec3(Properties::Position::PREFIX, pos);
}

void FlightDynamics::enforceConstraints() {
    Vec3 pos = m_state->getVec3(Properties::Position::PREFIX);
    if (pos.y < m_config.minAltitude) {
        pos.y = m_config.minAltitude;
        m_state->setVec3(Properties::Position::PREFIX, pos);
        double vertVel = m_state->get(Properties::Velocity::VERTICAL, 0.0);
        if (vertVel < 0.0) {
            m_state->set(Properties::Velocity::VERTICAL, 0.0);
        }
    }

    double speed = m_state->get(Properties::Velocity::AIRSPEED);
    speed = std::clamp(speed, static_cast<double>(m_config.minSpeed), 
                             static_cast<double>(m_config.maxSpeed));
    m_state->set(Properties::Velocity::AIRSPEED, speed);
}

}
