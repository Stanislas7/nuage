#include "aircraft/aircraft.hpp"
#include "core/property_paths.hpp"
#include "input/input_manager.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include "math/mat4.hpp"
#include "utils/config_loader.hpp"
#include "core/app.hpp" // Ensure App definition is available for assets()

#include "aircraft/systems/flight_dynamics/flight_dynamics.hpp"
#include "aircraft/systems/engine/engine_system.hpp"
#include "aircraft/systems/fuel/fuel_system.hpp"
#include "aircraft/systems/environment/environment_system.hpp"
#include <iostream>

namespace nuage {

void Aircraft::init(const std::string& configPath, App* app) {
    m_app = app;
    
    auto jsonOpt = loadJsonConfig(configPath);
    if (!jsonOpt) {
        // Fallback to default systems if config fails (or handle error)
        addSystem<FlightDynamics>();
        addSystem<EngineSystem>();
        addSystem<FuelSystem>();
        addSystem<EnvironmentSystem>(m_app->atmosphere());
        
        m_state.setVec3(Properties::Position::PREFIX, 0, 100, 0);
        m_state.set(Properties::Velocity::AIRSPEED, 50.0);
        m_state.setQuat(Properties::Orientation::PREFIX, Quat::identity());
        return;
    }

    const auto& json = *jsonOpt;

    // Load Model
    if (json.contains("model")) {
        const auto& mod = json["model"];
        std::string modelName = mod.value("name", "aircraft");
        std::string modelPath = mod.value("path", "");
        
        if (!modelPath.empty()) {
             if (m_app->assets().loadModel(modelName, modelPath)) {
                 m_mesh = m_app->assets().getMesh(modelName);
             } else {
                 std::cerr << "Failed to load aircraft model: " << modelPath << std::endl;
             }
        }

        if (mod.contains("color")) {
            auto c = mod["color"];
            if (c.is_array() && c.size() == 3) {
                m_color = Vec3(c[0], c[1], c[2]);
            }
        }
    }
    
    if (!m_mesh) {
        // Fallback to existing 'aircraft' mesh if defined, or null
        m_mesh = m_app->assets().getMesh("aircraft");
    }
    
    // Default shader
    m_shader = m_app->assets().getShader("basic");

    // Flight Dynamics
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

    // Engine
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

    // Fuel
    FuelConfig fuelConfig;
    if (json.contains("fuel")) {
        const auto& f = json["fuel"];
        fuelConfig.capacity = f.value("capacity", fuelConfig.capacity);
        fuelConfig.initialFuel = f.value("initial", fuelConfig.initialFuel);
    }
    addSystem<FuelSystem>(fuelConfig);

    // Environment
    addSystem<EnvironmentSystem>(m_app->atmosphere());

    // Initial State
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
    m_shader->setVec3("uColor", m_color);
    m_shader->setBool("uUseUniformColor", true);
    m_mesh->draw();
    m_shader->setBool("uUseUniformColor", false); // Reset
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
