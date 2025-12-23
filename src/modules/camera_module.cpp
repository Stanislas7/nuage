#include "modules/camera_module.hpp"
#include "core/simulator.hpp"
#include <cmath>

namespace flightsim {

void CameraModule::init(Simulator* sim) {
    Module::init(sim);
}

void CameraModule::update(float dt) {
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

void CameraModule::updateChaseCamera(float dt) {
    auto* player = m_sim->aircraft().player();
    if (!player) return;

    Vec3 targetPos = player->model.position();
    Vec3 targetForward = player->model.forward();

    // Desired camera position: behind and above
    Vec3 desiredPos = targetPos
                    - targetForward * m_chaseDistance
                    + Vec3{0, m_chaseHeight, 0};

    // Smooth interpolation
    float t = 1.0f - std::exp(-m_chaseSmoothing * dt);
    m_position = m_position + (desiredPos - m_position) * t;
    m_target = targetPos;
}

void CameraModule::updateFreeCamera(float dt) {
    // Free camera controlled by input (for debugging)
    // Could add WASD movement here
}

void CameraModule::buildMatrices() {
    m_context.view = Mat4::lookAt(m_position, m_target, Vec3{0, 1, 0});
    m_context.projection = Mat4::perspective(m_fov, m_aspect, m_near, m_far);
    m_context.viewProjection = m_context.projection * m_context.view;
    m_context.cameraPosition = m_position;
}

}
