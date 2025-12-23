#include "aircraft/Aircraft.hpp"
#include "managers/input/InputManager.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include "math/mat4.hpp"

namespace nuage {

void Aircraft::init(const std::string& configPath, App* app) {
    m_app = app;
    
    m_state.setVec3("position", 0, 100, 0);
    m_state.set("velocity/airspeed", 50.0);
    m_state.set("orientation/w", 1.0);
    m_state.set("orientation/x", 0.0);
    m_state.set("orientation/y", 0.0);
    m_state.set("orientation/z", 0.0);
}

void Aircraft::update(float dt, const FlightInput& input) {
    m_state.set("input/pitch", input.pitch);
    m_state.set("input/roll", input.roll);
    m_state.set("input/yaw", input.yaw);
    m_state.set("input/throttle", input.throttle);

    for (auto& system : m_systems) {
        system->update(dt);
    }
}

void Aircraft::render(const Mat4& viewProjection) {
    if (!m_mesh || !m_shader) return;

    Mat4 model = Mat4::translate(position()) * orientation().toMat4();
    m_shader->use();
    m_shader->setMat4("mvp", viewProjection * model);
    m_mesh->draw();
}

Vec3 Aircraft::position() const {
    return Vec3(
        m_state.get("position/x"),
        m_state.get("position/y"),
        m_state.get("position/z")
    );
}

Quat Aircraft::orientation() const {
    return Quat(
        m_state.get("orientation/w", 1.0),
        m_state.get("orientation/x"),
        m_state.get("orientation/y"),
        m_state.get("orientation/z")
    );
}

float Aircraft::airspeed() const {
    return static_cast<float>(m_state.get("velocity/airspeed"));
}

Vec3 Aircraft::forward() const {
    return orientation().rotate(Vec3(0, 0, 1));
}

Vec3 Aircraft::up() const {
    return orientation().rotate(Vec3(0, 1, 0));
}

Vec3 Aircraft::right() const {
    return orientation().rotate(Vec3(1, 0, 0));
}

}
