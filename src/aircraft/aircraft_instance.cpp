#include "aircraft/aircraft.hpp"
#include "core/property_paths.hpp"
#include "input/input.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include "graphics/asset_store.hpp"
#include "environment/atmosphere.hpp"
#include "math/mat4.hpp"
#include "utils/config_loader.hpp"

#include "aircraft/systems/physics/physics_integrator.hpp"
#include "aircraft/systems/physics/orientation_system.hpp"
#include "aircraft/systems/physics/gravity_system.hpp"
#include "aircraft/systems/physics/thrust_force.hpp"
#include "aircraft/systems/physics/lift_system.hpp"
#include "aircraft/systems/physics/drag_system.hpp"
#include "aircraft/systems/engine/engine_system.hpp"
#include "aircraft/systems/fuel/fuel_system.hpp"
#include "aircraft/systems/environment/environment_system.hpp"
#include <iostream>

namespace nuage {

void Aircraft::Instance::init(const std::string& configPath, AssetStore& assets, Atmosphere& atmosphere) {
    auto jsonOpt = loadJsonConfig(configPath);
    if (!jsonOpt) {
        addSystem<PhysicsIntegrator>();
        addSystem<EngineSystem>();
        addSystem<FuelSystem>();
        addSystem<EnvironmentSystem>(atmosphere);
        addSystem<OrientationSystem>();
        addSystem<GravitySystem>();
        addSystem<ThrustForce>();
        addSystem<LiftSystem>();
        addSystem<DragSystem>();

        m_state.set(Properties::Physics::MASS, 1000.0);
        m_state.setVec3(Properties::Position::PREFIX, 0, 100, 0);
        m_state.setVec3(Properties::Velocity::PREFIX, 0, 0, 50);
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
             if (assets.loadModel(modelName, modelPath)) {
                 m_mesh = assets.getMesh(modelName);
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
        m_mesh = assets.getMesh("aircraft");
    }

    m_shader = assets.getShader("basic");

    PhysicsConfig physicsConfig;
    if (json.contains("physics")) {
        const auto& phys = json["physics"];
        physicsConfig.minAltitude = phys.value("minAltitude", physicsConfig.minAltitude);
        physicsConfig.maxClimbRate = phys.value("maxClimbRate", physicsConfig.maxClimbRate);
    }
    addSystem<PhysicsIntegrator>(physicsConfig);

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

    addSystem<EnvironmentSystem>(atmosphere);

    OrientationConfig orientConfig;
    if (json.contains("orientation")) {
        const auto& orient = json["orientation"];
        orientConfig.pitchRate = orient.value("pitchRate", orientConfig.pitchRate);
        orientConfig.yawRate = orient.value("yawRate", orientConfig.yawRate);
        orientConfig.rollRate = orient.value("rollRate", orientConfig.rollRate);
    }
    addSystem<OrientationSystem>(orientConfig);

    float mass = 1000.0f;
    float cruiseSpeed = 50.0f;

    if (json.contains("physics")) {
        const auto& phys = json["physics"];
        mass = phys.value("mass", mass);
        cruiseSpeed = phys.value("cruiseSpeed", cruiseSpeed);
    }

    addSystem<GravitySystem>();

    ThrustConfig thrustConfig;
    if (json.contains("engineForce")) {
        const auto& ef = json["engineForce"];
        thrustConfig.thrustScale = ef.value("thrustScale", thrustConfig.thrustScale);
    }
    addSystem<ThrustForce>(thrustConfig);

    LiftConfig liftConfig;
    if (json.contains("lift")) {
        const auto& lift = json["lift"];
        liftConfig.liftCoefficient = lift.value("liftCoefficient", liftConfig.liftCoefficient);
        liftConfig.wingArea = lift.value("wingArea", liftConfig.wingArea);
    }
    addSystem<LiftSystem>(liftConfig);

    DragConfig dragConfig;
    if (json.contains("drag")) {
        const auto& drag = json["drag"];
        dragConfig.dragCoefficient = drag.value("dragCoefficient", dragConfig.dragCoefficient);
        dragConfig.frontalArea = drag.value("frontalArea", dragConfig.frontalArea);
    }
    addSystem<DragSystem>(dragConfig);

    // Initial State
    m_state.set(Properties::Physics::MASS, static_cast<double>(mass));
    m_state.setVec3(Properties::Position::PREFIX, 0, 100, 0);
    m_state.setVec3(Properties::Velocity::PREFIX, 0, 0, cruiseSpeed);
    m_state.setQuat(Properties::Orientation::PREFIX, Quat::identity());
}

void Aircraft::Instance::update(float dt, const FlightInput& input) {
    m_state.set(Properties::Input::PITCH, input.pitch);
    m_state.set(Properties::Input::ROLL, input.roll);
    m_state.set(Properties::Input::YAW, input.yaw);
    m_state.set(Properties::Input::THROTTLE, input.throttle);

    for (auto& system : m_systems) {
        system->update(dt);
    }
}

void Aircraft::Instance::render(const Mat4& viewProjection) {
    if (!m_mesh || !m_shader) return;

    Mat4 model = Mat4::translate(position()) * orientation().toMat4();
    m_shader->use();
    m_shader->setMat4("uMVP", viewProjection * model);
    m_shader->setVec3("uColor", m_color);
    m_shader->setBool("uUseUniformColor", true);
    m_mesh->draw();
    m_shader->setBool("uUseUniformColor", false); // Reset
}

Vec3 Aircraft::Instance::position() const {
    return m_state.getVec3(Properties::Position::PREFIX);
}

Quat Aircraft::Instance::orientation() const {
    return m_state.getQuat(Properties::Orientation::PREFIX);
}

float Aircraft::Instance::airspeed() const {
    return static_cast<float>(m_state.get(Properties::Physics::AIR_SPEED));
}

Vec3 Aircraft::Instance::forward() const {
    return orientation().rotate(Vec3(0, 0, 1));
}

Vec3 Aircraft::Instance::up() const {
    return orientation().rotate(Vec3(0, 1, 0));
}

Vec3 Aircraft::Instance::right() const {
    return orientation().rotate(Vec3(1, 0, 0));
}

}