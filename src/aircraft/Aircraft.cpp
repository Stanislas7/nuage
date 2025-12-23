#include "aircraft/Aircraft.hpp"
#include "aircraft/PropertyPaths.hpp"
#include "managers/input/InputManager.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include "math/mat4.hpp"
#include "utils/ConfigLoader.hpp"

#include "aircraft/systems/flight_dynamics/FlightDynamics.hpp"
#include "aircraft/systems/engine/EngineSystem.hpp"
#include "aircraft/systems/fuel/FuelSystem.hpp"
#include "aircraft/systems/environment/EnvironmentSystem.hpp"

namespace nuage {

void Aircraft::init(const std::string& configPath, App* app) {
    m_app = app;
    
    auto jsonOpt = loadJsonConfig(configPath);
    if (!jsonOpt) {
        // Fallback to default systems if config fails (or handle error)
        addSystem<FlightDynamics>();
        addSystem<EngineSystem>();
        addSystem<FuelSystem>();
        addSystem<EnvironmentSystem>();
        
        m_state.setVec3(Properties::Position::PREFIX, 0, 100, 0);
        m_state.set(Properties::Velocity::AIRSPEED, 50.0);
        m_state.setQuat(Properties::Orientation::PREFIX, Quat::identity());
        return;
    }

    const auto& json = *jsonOpt;

    FlightDynamicsConfig fdConfig;
    if (json.contains("flightDynamics")) {
        const auto& fd = json["flightDynamics"];
        fdConfig.minSpeed = fd.value("minSpeed", fdConfig.minSpeed);
        fdConfig.maxSpeed = fd.value("maxSpeed", fdConfig.maxSpeed);
        fdConfig.cruiseSpeed = fd.value("cruiseSpeed", fdConfig.cruiseSpeed);
        fdConfig.pitchRate = fd.value("pitchRate", fdConfig.pitchRate);
        fdConfig.yawRate = fd.value("yawRate", fdConfig.yawRate);
        fdConfig.rollRate = fd.value("rollRate", fdConfig.rollRate);
        fdConfig.maxPitch = fd.value("maxPitch", fdConfig.maxPitch);
        fdConfig.minAltitude = fd.value("minAltitude", fdConfig.minAltitude);
        fdConfig.throttleResponse = fd.value("throttleResponse", fdConfig.throttleResponse);
        fdConfig.liftCoefficient = fd.value("liftCoefficient", fdConfig.liftCoefficient);
        fdConfig.wingArea = fd.value("wingArea", fdConfig.wingArea);
    }
    addSystem<FlightDynamics>(fdConfig);

    EngineConfig engConfig;
    if (json.contains("engine")) {
        const auto& eng = json["engine"];
        engConfig.maxThrust = eng.value("maxThrust", engConfig.maxThrust);
        engConfig.idleN1 = eng.value("idleN1", engConfig.idleN1);
        engConfig.maxN1 = eng.value("maxN1", engConfig.maxN1);
        engConfig.spoolRate = eng.value("spoolRate", engConfig.spoolRate);
        engConfig.fuelFlowIdle = eng.value("fuelFlowIdle", engConfig.fuelFlowIdle);
        engConfig.fuelFlowMax = eng.value("fuelFlowMax", engConfig.fuelFlowMax);
    }
    addSystem<EngineSystem>(engConfig);

    FuelConfig fuelConfig;
    if (json.contains("fuel")) {
        const auto& f = json["fuel"];
        fuelConfig.capacity = f.value("capacity", fuelConfig.capacity);
        fuelConfig.initialFuel = f.value("initial", fuelConfig.initialFuel);
    }
    addSystem<FuelSystem>(fuelConfig);

    addSystem<EnvironmentSystem>();

    m_state.setVec3(Properties::Position::PREFIX, 0, 100, 0);
    m_state.set(Properties::Velocity::AIRSPEED, fdConfig.cruiseSpeed);
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
    m_shader->setMat4("uMVP", viewProjection * model);
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
