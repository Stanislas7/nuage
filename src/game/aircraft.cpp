#include "game/aircraft.hpp"
#include <cmath>
#include <algorithm>

namespace flightsim {

void Aircraft::init() {
    m_mesh.init(generateVertices());
}

void Aircraft::update(float dt, bool up, bool down, bool left, bool right, bool accel, bool decel) {
    if (left) m_yaw += 1.5f * dt;
    if (right) m_yaw -= 1.5f * dt;
    if (up) m_pitch -= 1.0f * dt;
    if (down) m_pitch += 1.0f * dt;
    if (accel) m_speed = std::min(m_speed + 20.0f * dt, 80.0f);
    if (decel) m_speed = std::max(m_speed - 20.0f * dt, 5.0f);

    m_pitch = std::max(-1.2f, std::min(1.2f, m_pitch));

    Vec3 forward(
        std::sin(m_yaw) * std::cos(m_pitch),
        std::sin(m_pitch),
        std::cos(m_yaw) * std::cos(m_pitch)
    );
    m_pos = m_pos + forward * (m_speed * dt);

    if (m_pos.y < 5) m_pos.y = 5;
}

void Aircraft::draw() const {
    m_mesh.draw();
}

Mat4 Aircraft::getModelMatrix() const {
    return Mat4::translate(m_pos.x, m_pos.y, m_pos.z) *
           Mat4::rotateY(m_yaw) *
           Mat4::rotateX(-m_pitch) *
           Mat4::scale(2.0f, 2.0f, 2.0f);
}

Vec3 Aircraft::getForward() const {
    return Vec3(std::sin(m_yaw), 0, std::cos(m_yaw));
}

std::vector<float> Aircraft::generateVertices() {
    Vec3 c1(0.8f, 0.2f, 0.2f);
    Vec3 c2(0.6f, 0.15f, 0.15f);
    Vec3 c3(0.3f, 0.3f, 0.4f);

    return {
        // Body bottom
        -1.0f, 0.0f, -0.5f, c1.x, c1.y, c1.z,
         1.0f, 0.0f, -0.5f, c1.x, c1.y, c1.z,
        -1.0f, 0.0f,  0.5f, c1.x, c1.y, c1.z,
         1.0f, 0.0f, -0.5f, c1.x, c1.y, c1.z,
         1.0f, 0.0f,  0.5f, c1.x, c1.y, c1.z,
        -1.0f, 0.0f,  0.5f, c1.x, c1.y, c1.z,

        // Body top
        -1.0f, 0.1f, -0.5f, c2.x, c2.y, c2.z,
         1.0f, 0.1f, -0.5f, c2.x, c2.y, c2.z,
        -1.0f, 0.1f,  0.5f, c2.x, c2.y, c2.z,
         1.0f, 0.1f, -0.5f, c2.x, c2.y, c2.z,
         1.0f, 0.1f,  0.5f, c2.x, c2.y, c2.z,
        -1.0f, 0.1f,  0.5f, c2.x, c2.y, c2.z,

        // Body left
        -1.0f, 0.0f, -0.5f, c2.x, c2.y, c2.z,
        -1.0f, 0.1f, -0.5f, c2.x, c2.y, c2.z,
        -1.0f, 0.0f,  0.5f, c2.x, c2.y, c2.z,
        -1.0f, 0.1f, -0.5f, c2.x, c2.y, c2.z,
        -1.0f, 0.1f,  0.5f, c2.x, c2.y, c2.z,
        -1.0f, 0.0f,  0.5f, c2.x, c2.y, c2.z,

        // Body right
         1.0f, 0.0f, -0.5f, c2.x, c2.y, c2.z,
         1.0f, 0.1f, -0.5f, c2.x, c2.y, c2.z,
         1.0f, 0.0f,  0.5f, c2.x, c2.y, c2.z,
         1.0f, 0.1f, -0.5f, c2.x, c2.y, c2.z,
         1.0f, 0.1f,  0.5f, c2.x, c2.y, c2.z,
         1.0f, 0.0f,  0.5f, c2.x, c2.y, c2.z,

        // Wings
        -2.5f, 0.05f, -0.1f, c3.x, c3.y, c3.z,
         2.5f, 0.05f, -0.1f, c3.x, c3.y, c3.z,
        -2.5f, 0.05f,  0.1f, c3.x, c3.y, c3.z,
         2.5f, 0.05f, -0.1f, c3.x, c3.y, c3.z,
         2.5f, 0.05f,  0.1f, c3.x, c3.y, c3.z,
        -2.5f, 0.05f,  0.1f, c3.x, c3.y, c3.z,

        // Tail
        -0.3f, 0.05f, -1.5f, c3.x, c3.y, c3.z,
         0.3f, 0.05f, -1.5f, c3.x, c3.y, c3.z,
        -0.1f, 0.05f, -0.5f, c3.x, c3.y, c3.z,
         0.3f, 0.05f, -1.5f, c3.x, c3.y, c3.z,
         0.1f, 0.05f, -0.5f, c3.x, c3.y, c3.z,
        -0.1f, 0.05f, -0.5f, c3.x, c3.y, c3.z,
    };
}

}
