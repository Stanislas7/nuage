#include "aircraft/Aircraft.hpp"
#include "aircraft/PropertyPaths.hpp"
#include "managers/input/InputManager.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include "math/mat4.hpp"

namespace nuage {

void Aircraft::init(const std::string& configPath, App* app) {
    m_app = app;
    
    m_state.setVec3(Properties::Position::PREFIX, 0, 100, 0);
    m_state.set(Properties::Velocity::AIRSPEED, 50.0);
    m_state.setQuat(Properties::Orientation::PREFIX, Quat::identity());
}

void Aircraft::update(float dt, const FlightInput& input) {
    m_state.set(Properties::Input::PITCH, input.pitch);
    m_state.set(Properties::Input::ROLL, input.roll);
    m_state.set(Properties::Input::YAW, input.yaw);
    m_state.set(Properties::Input::THROTTLE, input.throttle);

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
    return m_state.getVec3(Properties::Position::PREFIX);
}

Quat Aircraft::orientation() const {
    return m_state.getQuat(Properties::Orientation::PREFIX);
}

float Aircraft::airspeed() const {
    return static_cast<float>(m_state.get(Properties::Velocity::AIRSPEED));
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
