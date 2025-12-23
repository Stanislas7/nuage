#include "FlightDynamics.hpp"
#include "aircraft/property_bus.hpp"
#include "aircraft/PropertyPaths.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include <algorithm>
#include <cmath>

namespace nuage {

FlightDynamics::FlightDynamics(const FlightDynamicsConfig& config)
    : m_config(config)
{
}

void FlightDynamics::init(PropertyBus* state, App* app) {
    m_state = state;
    m_app = app;
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

    Vec3 pos = m_state->getVec3(Properties::Position::PREFIX);
    Vec3 newPos = pos + forward * static_cast<float>(speed * dt);
    m_state->setVec3(Properties::Position::PREFIX, newPos);
}

void FlightDynamics::enforceConstraints() {
    Vec3 pos = m_state->getVec3(Properties::Position::PREFIX);
    if (pos.y < m_config.minAltitude) {
        pos.y = m_config.minAltitude;
        m_state->setVec3(Properties::Position::PREFIX, pos);
    }

    double speed = m_state->get(Properties::Velocity::AIRSPEED);
    speed = std::clamp(speed, static_cast<double>(m_config.minSpeed), 
                             static_cast<double>(m_config.maxSpeed));
    m_state->set(Properties::Velocity::AIRSPEED, speed);
}

}
