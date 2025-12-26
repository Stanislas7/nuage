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

void Camera::update(float dt, Aircraft::Instance* target, float alpha) {
    // If a target is provided, use it; otherwise use the internal one if set.
    if (target) {
        m_target = target;
    }

    switch (m_mode) {
        case CameraMode::Chase:
            updateChaseCamera(dt, m_target, alpha);
            break;
        case CameraMode::Orbit:
            updateOrbitCamera(dt, m_target, alpha);
            break;
        default:
            break;
    }

    buildMatrices();
}

void Camera::updateChaseCamera(float dt, Aircraft::Instance* target, float alpha) {
    if (!target) {
        return;
    }

    Vec3 targetPos = target->interpolatedPosition(alpha);
    Vec3 targetForward = target->interpolatedOrientation(alpha).rotate(Vec3(0, 0, 1));

    float forwardLen = std::sqrt(targetForward.x * targetForward.x + 
                                targetForward.y * targetForward.y + 
                                targetForward.z * targetForward.z);
    if (forwardLen < 0.001f) {
        targetForward = Vec3(0, 0, 1);
    } else {
        targetForward = targetForward * (1.0f / forwardLen);
    }

    float tForward = 1.0f - std::exp(-m_forwardSmoothing * dt);
    m_smoothedForward = m_smoothedForward + (targetForward - m_smoothedForward) * tForward;

    float smoothForwardLen = std::sqrt(m_smoothedForward.x * m_smoothedForward.x + 
                                       m_smoothedForward.y * m_smoothedForward.y + 
                                       m_smoothedForward.z * m_smoothedForward.z);
    if (smoothForwardLen > 0.001f) {
        m_smoothedForward = m_smoothedForward * (1.0f / smoothForwardLen);
    }

    Vec3 desiredPos = targetPos
                    - m_smoothedForward * m_followDistance
                    + Vec3(0, m_followHeight, 0);

    float tPos = 1.0f - std::exp(-m_positionSmoothing * dt);
    tPos = std::max(0.0f, std::min(1.0f, tPos));

    Vec3 posDiff = desiredPos - m_position;
    float posDiffLen = std::sqrt(posDiff.x * posDiff.x + posDiff.y * posDiff.y + posDiff.z * posDiff.z);
    const float deadZone = 0.001f;
    if (posDiffLen > deadZone) {
        m_position = m_position + posDiff * tPos;
    }

    float tLookAt = 1.0f - std::exp(-m_lookAtSmoothing * dt);
    tLookAt = std::max(0.0f, std::min(1.0f, tLookAt));

    Vec3 lookAtDiff = targetPos - m_smoothedLookAt;
    float lookAtDiffLen = std::sqrt(lookAtDiff.x * lookAtDiff.x + 
                                    lookAtDiff.y * lookAtDiff.y + 
                                    lookAtDiff.z * lookAtDiff.z);
    if (lookAtDiffLen > deadZone) {
        m_smoothedLookAt = m_smoothedLookAt + lookAtDiff * tLookAt;
    }

    m_lookAt = m_smoothedLookAt;
}

void Camera::updateOrbitCamera(float dt, Aircraft::Instance* target, float alpha) {
    if (!target) return;

    Vec2 mouseDelta = m_input->mouseDelta();

    m_orbitYaw += mouseDelta.x * m_orbitSpeed * dt;
    m_orbitPitch -= mouseDelta.y * m_orbitSpeed * dt;
    m_orbitPitch = std::max(-1.5f, std::min(1.5f, m_orbitPitch));

    Vec3 targetPos = target->interpolatedPosition(alpha);

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