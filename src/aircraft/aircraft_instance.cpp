#include "aircraft/aircraft.hpp"
#include "core/property_paths.hpp"
#include "input/input.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/texture.hpp"
#include "graphics/model.hpp"
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
#include "aircraft/systems/physics/stability_system.hpp"
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

    void finalizeLiftConfig(LiftConfig& config) {
        if (config.stallAlphaRad <= 0.0f && std::abs(config.clAlpha) > 0.0001f) {
            config.stallAlphaRad = (config.clMax - config.cl0) / config.clAlpha;
        }
        if (config.stallAlphaRad <= 0.0f) {
            config.stallAlphaRad = 0.35f;
        }
        if (config.postStallAlphaRad <= config.stallAlphaRad) {
            config.postStallAlphaRad = config.stallAlphaRad + 0.35f;
        }
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
        m_model = assets.getModel(modelName);
        if (!m_model || m_model->parts().empty()) {
            m_mesh = assets.getMesh(modelName);
        }
    }

    auto c = mod[ConfigKeys::COLOR];
    m_color = Vec3(c[0], c[1], c[2]);

    std::string texturePath = mod.contains(ConfigKeys::TEXTURE) ? mod[ConfigKeys::TEXTURE].get<std::string>() : modelTexturePath;
    if ((!m_model || m_model->parts().empty()) && !texturePath.empty() && modelHasTexcoords) {
        std::string textureName = modelName + "_diffuse";
        if (assets.loadTexture(textureName, texturePath)) {
            m_texture = assets.getTexture(textureName);
        }
    }
    
    m_modelScale = parseScale(mod[ConfigKeys::SCALE], m_modelScale);
    m_modelRotation = parseRotation(mod[ConfigKeys::ROTATION], m_modelRotation);
    m_modelOffset = parseVec3(mod[ConfigKeys::OFFSET], m_modelOffset);
    
    if (!m_mesh && (!m_model || m_model->parts().empty())) {
        m_mesh = assets.getMesh("aircraft");
    }
    m_shader = assets.getShader("basic");
    m_texturedShader = assets.getShader("textured");

    // Physics
    const auto& phys = json[ConfigKeys::PHYSICS];
    PhysicsConfig physicsConfig;
    physicsConfig.minAltitude = phys[ConfigKeys::MIN_ALTITUDE];
    physicsConfig.maxClimbRate = phys[ConfigKeys::MAX_CLIMB_RATE];
    physicsConfig.groundFriction = phys[ConfigKeys::GROUND_FRICTION];
    physicsConfig.inertia = parseVec3(phys[ConfigKeys::INERTIA], physicsConfig.inertia);
    // PhysicsIntegrator moved to end


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

    const auto& stab = json[ConfigKeys::STABILITY];
    StabilityConfig stabilityConfig;
    stabilityConfig.pitchStability = stab[ConfigKeys::PITCH_STABILITY];
    stabilityConfig.yawStability = stab[ConfigKeys::YAW_STABILITY];
    stabilityConfig.rollStability = stab[ConfigKeys::ROLL_STABILITY];
    stabilityConfig.pitchDamping = stab[ConfigKeys::PITCH_DAMPING];
    stabilityConfig.yawDamping = stab[ConfigKeys::YAW_DAMPING];
    stabilityConfig.rollDamping = stab[ConfigKeys::ROLL_DAMPING];
    stabilityConfig.referenceArea = stab[ConfigKeys::REFERENCE_AREA];
    stabilityConfig.referenceLength = stab[ConfigKeys::REFERENCE_LENGTH];
    stabilityConfig.momentScale = stab[ConfigKeys::MOMENT_SCALE];
    stabilityConfig.minAirspeed = stab[ConfigKeys::STABILITY_MIN_AIRSPEED];
    addSystem<StabilitySystem>(stabilityConfig);

    // Forces
    addSystem<GravitySystem>();

    const auto& ef = json[ConfigKeys::ENGINE_FORCE];
    ThrustConfig thrustConfig;
    thrustConfig.thrustScale = ef[ConfigKeys::THRUST_SCALE];
    thrustConfig.propEfficiency = ef[ConfigKeys::PROP_EFFICIENCY];
    thrustConfig.minAirspeed = ef[ConfigKeys::MIN_AIRSPEED];
    thrustConfig.maxStaticThrust = ef[ConfigKeys::MAX_STATIC_THRUST];
    addSystem<ThrustForce>(thrustConfig);

    const auto& lift = json[ConfigKeys::LIFT];
    LiftConfig liftConfig;
    liftConfig.cl0 = lift[ConfigKeys::CL0];
    liftConfig.clAlpha = lift[ConfigKeys::CL_ALPHA];
    liftConfig.clMax = lift[ConfigKeys::CL_MAX];
    liftConfig.clMin = lift[ConfigKeys::CL_MIN];
    liftConfig.wingArea = lift[ConfigKeys::WING_AREA];
    liftConfig.stallAlphaRad = lift.value(ConfigKeys::STALL_ALPHA_DEG, 0.0f) * kDegToRad;
    liftConfig.postStallAlphaRad = lift.value(ConfigKeys::POST_STALL_ALPHA_DEG, 0.0f) * kDegToRad;
    liftConfig.clPostStall = lift.value(ConfigKeys::CL_POST_STALL, liftConfig.clPostStall);
    liftConfig.clPostStallNeg = lift.value(ConfigKeys::CL_POST_STALL_NEG, liftConfig.clPostStallNeg);
    finalizeLiftConfig(liftConfig);
    addSystem<LiftSystem>(liftConfig);

    const auto& drag = json[ConfigKeys::DRAG];
    DragConfig dragConfig;
    dragConfig.cd0 = drag[ConfigKeys::CD0];
    dragConfig.inducedDragFactor = drag[ConfigKeys::INDUCED_DRAG_FACTOR];
    dragConfig.wingArea = drag[ConfigKeys::WING_AREA];
    dragConfig.frontalArea = drag.value(ConfigKeys::FRONTAL_AREA, dragConfig.frontalArea);
    dragConfig.cdStall = drag.value(ConfigKeys::CD_STALL, dragConfig.cdStall);
    dragConfig.stallAlphaRad = liftConfig.stallAlphaRad;
    dragConfig.postStallAlphaRad = liftConfig.postStallAlphaRad;
    addSystem<DragSystem>(dragConfig);

    // Initial State
    m_state.set(Properties::Physics::MASS, static_cast<double>(phys[ConfigKeys::MASS]));
    
    const auto& spawn = json[ConfigKeys::SPAWN];
    Vec3 initialPos = parseVec3(spawn[ConfigKeys::POSITION], Vec3(0, 100, 0));
    Vec3 initialVel(0, 0, spawn[ConfigKeys::AIRSPEED]);
    
    m_state.setVec3(Properties::Position::PREFIX, initialPos);
    m_state.setVec3(Properties::Velocity::PREFIX, initialVel);
    m_state.setQuat(Properties::Orientation::PREFIX, Quat::identity());
    
    // Initialize previous state
    m_prevState = m_state;

    // Add Integrator last (Semi-Implicit: Calculate Forces -> Integrate)
    addSystem<PhysicsIntegrator>(physicsConfig);
}

void Aircraft::Instance::update(float dt, const FlightInput& input) {
    // Save previous state for interpolation
    m_prevState = m_state;

    m_state.set(Properties::Input::PITCH, input.pitch);
    m_state.set(Properties::Input::ROLL, input.roll);
    m_state.set(Properties::Input::YAW, input.yaw);
    m_state.set(Properties::Input::THROTTLE, input.throttle);

    // Clear forces and torques for the new frame accumulation
    m_state.setVec3(Properties::Physics::FORCE_PREFIX, 0.0f, 0.0f, 0.0f);
    m_state.setVec3(Properties::Physics::TORQUE_PREFIX, 0.0f, 0.0f, 0.0f);

    for (auto& system : m_systems) {
        system->update(dt);
    }
}

void Aircraft::Instance::render(const Mat4& viewProjection, float alpha) {
    Vec3 renderPos = interpolatedPosition(alpha);
    Quat renderRot = interpolatedOrientation(alpha);

    Mat4 model = Mat4::translate(renderPos)
        * renderRot.toMat4()
        * Mat4::translate(m_modelOffset)
        * m_modelRotation.toMat4()
        * Mat4::scale(m_modelScale.x, m_modelScale.y, m_modelScale.z);

    if (m_model && !m_model->parts().empty()) {
        for (const auto& part : m_model->parts()) {
            if (!part.mesh) continue;
            Shader* shader = (part.textured && part.texture && m_texturedShader) ? m_texturedShader : m_shader;
            if (!shader) continue;
            shader->use();
            shader->setMat4("uMVP", viewProjection * model);
            if (part.textured && part.texture && shader == m_texturedShader) {
                part.texture->bind(0);
                shader->setInt("uTexture", 0);
            } else {
                shader->setVec3("uColor", m_color);
                shader->setBool("uUseUniformColor", true);
            }
            part.mesh->draw();
            if (shader == m_shader) {
                shader->setBool("uUseUniformColor", false);
            }
        }
        return;
    }

    if (!m_mesh || !m_shader) return;

    m_shader->use();
    m_shader->setMat4("uMVP", viewProjection * model);
    if (m_texture && m_texturedShader) {
        m_texturedShader->use();
        m_texturedShader->setMat4("uMVP", viewProjection * model);
        m_texture->bind(0);
        m_texturedShader->setInt("uTexture", 0);
        m_mesh->draw();
        return;
    }
    m_shader->setVec3("uColor", m_color);
    m_shader->setBool("uUseUniformColor", true);
    m_mesh->draw();
    m_shader->setBool("uUseUniformColor", false);
}

Vec3 Aircraft::Instance::position() const {
    return m_state.getVec3(Properties::Position::PREFIX);
}

Quat Aircraft::Instance::orientation() const {
    return m_state.getQuat(Properties::Orientation::PREFIX);
}

Vec3 Aircraft::Instance::interpolatedPosition(float alpha) const {
    Vec3 curr = m_state.getVec3(Properties::Position::PREFIX);
    Vec3 prev = m_prevState.getVec3(Properties::Position::PREFIX);
    return prev + (curr - prev) * alpha;
}

Quat Aircraft::Instance::interpolatedOrientation(float alpha) const {
    Quat curr = m_state.getQuat(Properties::Orientation::PREFIX);
    Quat prev = m_prevState.getQuat(Properties::Orientation::PREFIX);
    return Quat::slerp(prev, curr, alpha);
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
