#include "graphics/camera.hpp"
#include "aircraft/aircraft.hpp"
#include "input/input.hpp"
#include <cmath>
#include <iostream>
#include <GLFW/glfw3.h> // For cursor constants

namespace nuage {

void Camera::init(Input& input) {
    m_input = &input;
}

void Camera::update(float dt, Aircraft::Instance* target) {
    // If a target is provided, use it; otherwise use the internal one if set.
    if (target) {
        m_target = target;
    }

    switch (m_mode) {
        case CameraMode::Chase:
            updateChaseCamera(dt, m_target);
            break;
        case CameraMode::Orbit:
            updateOrbitCamera(dt, m_target);
            break;
        default:
            break;
    }

    buildMatrices();
}

void Camera::updateChaseCamera(float dt, Aircraft::Instance* target) {
    if (!target) {
        // No target, maybe just stay still or handle gracefully
        return;
    }

    Vec3 targetPos = target->position();
    Vec3 targetForward = target->forward();

    Vec3 desiredPos = targetPos
                    - targetForward * m_followDistance
                    + Vec3(0, m_followHeight, 0);

    float t = 1.0f - std::exp(-m_smoothing * dt);
    m_position = m_position + (desiredPos - m_position) * t;
    m_lookAt = targetPos;
}

void Camera::updateOrbitCamera(float dt, Aircraft::Instance* target) {
    if (!target) return;

    Vec2 mouseDelta = m_input->mouseDelta();

    m_orbitYaw += mouseDelta.x * m_orbitSpeed * dt;
    m_orbitPitch -= mouseDelta.y * m_orbitSpeed * dt;
    m_orbitPitch = std::max(-1.5f, std::min(1.5f, m_orbitPitch));

    Vec3 targetPos = target->position();

    float x = m_orbitDistance * std::cos(m_orbitPitch) * std::sin(m_orbitYaw);
    float y = m_orbitDistance * std::sin(m_orbitPitch);
    float z = m_orbitDistance * std::cos(m_orbitPitch) * std::cos(m_orbitYaw);

    m_position = targetPos + Vec3(x, y, z);
    m_lookAt = targetPos;
}

void Camera::toggleOrbitMode() {
    if (m_mode == CameraMode::Orbit) {
        m_mode = CameraMode::Chase;
        m_input->setCursorMode(GLFW_CURSOR_NORMAL);
    } else {
        m_mode = CameraMode::Orbit;
        m_input->setCursorMode(GLFW_CURSOR_DISABLED);
        m_input->centerCursor();
    }
}

void Camera::buildMatrices() {
    m_view = Mat4::lookAt(m_position, m_lookAt, Vec3(0, 1, 0));
    m_projection = Mat4::perspective(m_fov, m_aspect, m_near, m_far);
}

}