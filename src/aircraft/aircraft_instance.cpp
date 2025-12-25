#include "aircraft/aircraft.hpp"
#include "core/property_paths.hpp"
#include "input/input.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/texture.hpp"
#include "environment/atmosphere.hpp"
#include "math/mat4.hpp"
#include "utils/config_loader.hpp"
#include "aircraft/aircraft_config_keys.hpp"

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
#include <cmath>

namespace nuage {

namespace {
    constexpr float kDegToRad = 3.1415926535f / 180.0f;

    Vec3 parseVec3(const json& value, const Vec3& fallback) {
        if (!value.is_array() || value.size() != 3) return fallback;
        return Vec3(
            value[0].get<float>(),
            value[1].get<float>(),
            value[2].get<float>()
        );
    }

    Vec3 parseScale(const json& value, const Vec3& fallback) {
        if (value.is_number()) {
            float s = value.get<float>();
            return Vec3(s, s, s);
        }
        return parseVec3(value, fallback);
    }

    Quat parseRotation(const json& value, const Quat& fallback) {
        if (!value.is_array() || value.size() != 3) return fallback;
        float rx = value[0].get<float>() * kDegToRad;
        float ry = value[1].get<float>() * kDegToRad;
        float rz = value[2].get<float>() * kDegToRad;
        Quat qx = Quat::fromAxisAngle(Vec3(1, 0, 0), rx);
        Quat qy = Quat::fromAxisAngle(Vec3(0, 1, 0), ry);
        Quat qz = Quat::fromAxisAngle(Vec3(0, 0, 1), rz);
        return (qz * qy * qx).normalized();
    }
}

void Aircraft::Instance::init(const std::string& configPath, AssetStore& assets, Atmosphere& atmosphere) {
    auto jsonOpt = loadJsonConfig(configPath);
    if (!jsonOpt) {
        // Fallback for missing file - this part we keep for basic safety but simplified
        addSystem<PhysicsIntegrator>();
        addSystem<EngineSystem>();
        addSystem<FuelSystem>();
        addSystem<EnvironmentSystem>(atmosphere);
        addSystem<OrientationSystem>();
        addSystem<GravitySystem>();
        addSystem<ThrustForce>();
        addSystem<LiftSystem>();
        addSystem<DragSystem>();
        return;
    }

    const auto& json = *jsonOpt;

    // Load Model
    const auto& mod = json[ConfigKeys::MODEL];
    std::string modelName = mod[ConfigKeys::NAME];
    std::string modelPath = mod[ConfigKeys::PATH];
    std::string modelTexturePath;
    bool modelHasTexcoords = false;
    
    if (assets.loadModel(modelName, modelPath, &modelTexturePath, &modelHasTexcoords)) {
        m_mesh = assets.getMesh(modelName);
    }

    auto c = mod[ConfigKeys::COLOR];
    m_color = Vec3(c[0], c[1], c[2]);

    std::string texturePath = mod.contains(ConfigKeys::TEXTURE) ? mod[ConfigKeys::TEXTURE].get<std::string>() : modelTexturePath;
    if (!texturePath.empty() && modelHasTexcoords) {
        std::string textureName = modelName + "_diffuse";
        if (assets.loadTexture(textureName, texturePath)) {
            m_texture = assets.getTexture(textureName);
            m_shader = assets.getShader("textured");
        }
    }
    
    m_modelScale = parseScale(mod[ConfigKeys::SCALE], m_modelScale);
    m_modelRotation = parseRotation(mod[ConfigKeys::ROTATION], m_modelRotation);
    m_modelOffset = parseVec3(mod[ConfigKeys::OFFSET], m_modelOffset);
    
    if (!m_mesh) m_mesh = assets.getMesh("aircraft");
    if (!m_shader) m_shader = assets.getShader("basic");

    // Physics
    const auto& phys = json[ConfigKeys::PHYSICS];
    PhysicsConfig physicsConfig;
    physicsConfig.minAltitude = phys[ConfigKeys::MIN_ALTITUDE];
    physicsConfig.maxClimbRate = phys[ConfigKeys::MAX_CLIMB_RATE];
    physicsConfig.groundFriction = phys[ConfigKeys::GROUND_FRICTION];
    physicsConfig.inertia = parseVec3(phys[ConfigKeys::INERTIA], physicsConfig.inertia);
    addSystem<PhysicsIntegrator>(physicsConfig);

    // Engine
    const auto& eng = json[ConfigKeys::ENGINE];
    EngineConfig engConfig;
    engConfig.maxThrust = eng[ConfigKeys::MAX_THRUST];
    engConfig.maxPowerKw = eng[ConfigKeys::MAX_POWER_KW];
    engConfig.idleN1 = eng[ConfigKeys::IDLE_N1];
    engConfig.maxN1 = eng[ConfigKeys::MAX_N1];
    engConfig.spoolRate = eng[ConfigKeys::SPOOL_RATE];
    engConfig.fuelFlowIdle = eng[ConfigKeys::FUEL_FLOW_IDLE];
    engConfig.fuelFlowMax = eng[ConfigKeys::FUEL_FLOW_MAX];
    addSystem<EngineSystem>(engConfig);

    // Fuel
    const auto& f = json[ConfigKeys::FUEL];
    FuelConfig fuelConfig;
    fuelConfig.capacity = f[ConfigKeys::CAPACITY];
    fuelConfig.initialFuel = f[ConfigKeys::INITIAL];
    addSystem<FuelSystem>(fuelConfig);

    addSystem<EnvironmentSystem>(atmosphere);

    // Orientation
    const auto& orient = json[ConfigKeys::ORIENTATION];
    OrientationConfig orientConfig;
    orientConfig.pitchRate = orient[ConfigKeys::PITCH_RATE];
    orientConfig.yawRate = orient[ConfigKeys::YAW_RATE];
    orientConfig.rollRate = orient[ConfigKeys::ROLL_RATE];
    orientConfig.controlRefSpeed = orient[ConfigKeys::CONTROL_REF_SPEED];
    orientConfig.minControlScale = orient[ConfigKeys::MIN_CONTROL_SCALE];
    orientConfig.maxControlScale = orient[ConfigKeys::MAX_CONTROL_SCALE];
    orientConfig.torqueMultiplier = orient[ConfigKeys::TORQUE_MULTIPLIER];
    orientConfig.dampingFactor = orient[ConfigKeys::DAMPING_FACTOR];
    addSystem<OrientationSystem>(orientConfig);

    // Forces
    addSystem<GravitySystem>();

    const auto& ef = json[ConfigKeys::ENGINE_FORCE];
    ThrustConfig thrustConfig;
    thrustConfig.thrustScale = ef[ConfigKeys::THRUST_SCALE];
    thrustConfig.propEfficiency = ef[ConfigKeys::PROP_EFFICIENCY];
    thrustConfig.minAirspeed = ef[ConfigKeys::MIN_AIRSPEED];
    addSystem<ThrustForce>(thrustConfig);

    const auto& lift = json[ConfigKeys::LIFT];
    LiftConfig liftConfig;
    liftConfig.cl0 = lift[ConfigKeys::CL0];
    liftConfig.clAlpha = lift[ConfigKeys::CL_ALPHA];
    liftConfig.clMax = lift[ConfigKeys::CL_MAX];
    liftConfig.clMin = lift[ConfigKeys::CL_MIN];
    liftConfig.wingArea = lift[ConfigKeys::WING_AREA];
    addSystem<LiftSystem>(liftConfig);

    const auto& drag = json[ConfigKeys::DRAG];
    DragConfig dragConfig;
    dragConfig.cd0 = drag[ConfigKeys::CD0];
    dragConfig.inducedDragFactor = drag[ConfigKeys::INDUCED_DRAG_FACTOR];
    dragConfig.wingArea = drag[ConfigKeys::WING_AREA];
    addSystem<DragSystem>(dragConfig);

    // Initial State
    m_state.set(Properties::Physics::MASS, static_cast<double>(phys[ConfigKeys::MASS]));
    
    const auto& spawn = json[ConfigKeys::SPAWN];
    Vec3 initialPos = parseVec3(spawn[ConfigKeys::POSITION], Vec3(0, 100, 0));
    Vec3 initialVel(0, 0, spawn[ConfigKeys::AIRSPEED]);
    
    m_state.setVec3(Properties::Position::PREFIX, initialPos);
    m_state.setVec3(Properties::Velocity::PREFIX, initialVel);
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

    Mat4 model = Mat4::translate(position())
        * orientation().toMat4()
        * Mat4::translate(m_modelOffset)
        * m_modelRotation.toMat4()
        * Mat4::scale(m_modelScale.x, m_modelScale.y, m_modelScale.z);
    m_shader->use();
    m_shader->setMat4("uMVP", viewProjection * model);
    if (m_texture) {
        m_texture->bind(0);
        m_shader->setInt("uTexture", 0);
    } else {
        m_shader->setVec3("uColor", m_color);
        m_shader->setBool("uUseUniformColor", true);
    }
    m_mesh->draw();
    if (!m_texture) {
        m_shader->setBool("uUseUniformColor", false); // Reset
    }
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
