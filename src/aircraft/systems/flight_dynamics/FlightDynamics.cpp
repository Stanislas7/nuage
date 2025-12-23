#include "FlightDynamics.hpp"
#include "aircraft/property_bus.hpp"
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
    double throttleInput = m_state->get("input/throttle");
    double currentSpeed = m_state->get("velocity/airspeed", m_config.minSpeed);

    double targetSpeed = m_config.minSpeed +
        (m_config.maxSpeed - m_config.minSpeed) * throttleInput;

    double speedDelta = (targetSpeed - currentSpeed) * m_config.throttleResponse * dt;
    m_state->set("velocity/airspeed", currentSpeed + speedDelta);
}

void FlightDynamics::updateOrientation(float dt) {
    double pitch = m_state->get("input/pitch");
    double yaw = m_state->get("input/yaw");
    double roll = m_state->get("input/roll");

    Quat orientation(
        m_state->get("orientation/w", 1.0),
        m_state->get("orientation/x"),
        m_state->get("orientation/y"),
        m_state->get("orientation/z")
    );

    Vec3 fwd = orientation.rotate(Vec3(0, 0, 1));
    Vec3 rgt = orientation.rotate(Vec3(1, 0, 0));

    float pitchDelta = static_cast<float>(pitch * m_config.pitchRate * dt);
    float yawDelta = static_cast<float>(yaw * m_config.yawRate * dt);
    float rollDelta = static_cast<float>(roll * m_config.rollRate * dt);

    Quat pitchRot = Quat::fromAxisAngle(rgt, pitchDelta);
    Quat yawRot = Quat::fromAxisAngle(Vec3(0, 1, 0), yawDelta);
    Quat rollRot = Quat::fromAxisAngle(fwd, rollDelta);

    Quat newOrientation = (yawRot * pitchRot * rollRot * orientation).normalized();

    m_state->set("orientation/w", newOrientation.w);
    m_state->set("orientation/x", newOrientation.x);
    m_state->set("orientation/y", newOrientation.y);
    m_state->set("orientation/z", newOrientation.z);
}

void FlightDynamics::updatePosition(float dt) {
    Quat orientation(
        m_state->get("orientation/w", 1.0),
        m_state->get("orientation/x"),
        m_state->get("orientation/y"),
        m_state->get("orientation/z")
    );

    Vec3 forward = orientation.rotate(Vec3(0, 0, 1));
    double speed = m_state->get("velocity/airspeed");

    Vec3 pos(
        m_state->get("position/x"),
        m_state->get("position/y"),
        m_state->get("position/z")
    );

    Vec3 newPos = pos + forward * static_cast<float>(speed * dt);
    m_state->setVec3("position", newPos.x, newPos.y, newPos.z);
}

void FlightDynamics::enforceConstraints() {
    double y = m_state->get("position/y");
    if (y < m_config.minAltitude) {
        m_state->set("position/y", m_config.minAltitude);
    }

    double speed = m_state->get("velocity/airspeed");
    speed = std::clamp(speed, static_cast<double>(m_config.minSpeed), 
                             static_cast<double>(m_config.maxSpeed));
    m_state->set("velocity/airspeed", speed);
}

}
