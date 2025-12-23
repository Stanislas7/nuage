#include "aircraft/flight_model.hpp"
#include <algorithm>
#include <cmath>

namespace flightsim {

void FlightModel::init(const FlightParams& params) {
    m_params = params;
    m_state = AircraftState{};
}

void FlightModel::update(float dt, const FlightControls& controls) {
    updateThrottle(dt, controls.throttle);
    updateOrientation(dt, controls);
    updatePosition(dt);
    enforceConstraints();
}

void FlightModel::updateThrottle(float dt, float throttleInput) {
    // Throttle directly from input (already accumulated in InputModule)
    m_state.throttle = std::clamp(throttleInput, 0.0f, 1.0f);

    // Speed based on throttle
    float targetSpeed = m_params.minSpeed +
        (m_params.maxSpeed - m_params.minSpeed) * m_state.throttle;

    // Smooth speed changes
    float speedDelta = (targetSpeed - m_state.speed) * m_params.throttleResponse * dt;
    m_state.speed = m_state.speed + speedDelta;
}

void FlightModel::updateOrientation(float dt, const FlightControls& controls) {
    // Apply control inputs as rotation rates
    float pitchDelta = controls.pitch * m_params.pitchRate * dt;
    float yawDelta = controls.yaw * m_params.yawRate * dt;
    float rollDelta = controls.roll * m_params.rollRate * dt;

    // Apply rotations in local space
    Quat pitchRot = Quat::fromAxisAngle(right(), pitchDelta);
    Quat yawRot = Quat::fromAxisAngle(Vec3{0, 1, 0}, yawDelta);
    Quat rollRot = Quat::fromAxisAngle(forward(), rollDelta);

    m_state.orientation = (yawRot * pitchRot * rollRot * m_state.orientation).normalized();
}

void FlightModel::updatePosition(float dt) {
    Vec3 velocity = forward() * m_state.speed;
    m_state.position = m_state.position + velocity * dt;
}

void FlightModel::enforceConstraints() {
    // Ground collision
    if (m_state.position.y < m_params.minAltitude) {
        m_state.position.y = m_params.minAltitude;
    }

    // Speed limits
    m_state.speed = std::clamp(m_state.speed, m_params.minSpeed, m_params.maxSpeed);
}

Vec3 FlightModel::forward() const {
    return m_state.orientation.rotate(Vec3{0, 0, 1});
}

Vec3 FlightModel::up() const {
    return m_state.orientation.rotate(Vec3{0, 1, 0});
}

Vec3 FlightModel::right() const {
    return m_state.orientation.rotate(Vec3{1, 0, 0});
}

}
