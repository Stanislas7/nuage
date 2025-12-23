#include "managers/camera/CameraManager.hpp"
#include "app/App.hpp"
#include "aircraft/Aircraft.hpp"
#include <cmath>
#include <iostream>

namespace nuage {

void CameraManager::init(App* app) {
    m_app = app;
}

void CameraManager::update(float dt) {
    switch (m_mode) {
        case CameraMode::Chase:
            updateChaseCamera(dt);
            break;
        case CameraMode::Free:
            updateFreeCamera(dt);
            break;
        default:
            break;
    }

    buildMatrices();
}

void CameraManager::updateChaseCamera(float dt) {
    if (!m_target) {
        m_target = m_app->aircraft().player();
        if (m_target) {
            std::cout << "[CAMERA] Target acquired: player aircraft\n";
        } else {
            std::cout << "[CAMERA] WARNING: No player aircraft found!\n";
            return;
        }
    }

    Vec3 targetPos = m_target->position();
    Vec3 targetForward = m_target->forward();

    Vec3 desiredPos = targetPos
                    - targetForward * m_followDistance
                    + Vec3(0, m_followHeight, 0);

    float t = 1.0f - std::exp(-m_smoothing * dt);
    m_position = m_position + (desiredPos - m_position) * t;
    m_lookAt = targetPos;

    static float debugTimer = 0;
    debugTimer += dt;
    if (debugTimer > 2.0f) {
        debugTimer = 0;
        std::cout << "[CAMERA] pos=(" << m_position.x << ", " << m_position.y << ", " << m_position.z << ")"
                  << " target=(" << targetPos.x << ", " << targetPos.y << ", " << targetPos.z << ")\n" << std::flush;
    }
}

void CameraManager::updateFreeCamera(float dt) {
    // Free camera for debugging
}

void CameraManager::buildMatrices() {
    m_view = Mat4::lookAt(m_position, m_lookAt, Vec3(0, 1, 0));
    m_projection = Mat4::perspective(m_fov, m_aspect, m_near, m_far);
}

}
