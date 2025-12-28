#include "aircraft/aircraft.hpp"
#include "core/properties/property_paths.hpp"
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

#include "aircraft/systems/physics/jsbsim_system.hpp"
#include "aircraft/systems/environment/environment_system.hpp"
#include <iostream>

namespace nuage {

void Aircraft::Instance::init(const std::string& configPath, AssetStore& assets, Atmosphere& atmosphere) {
    auto jsonOpt = loadJsonConfig(configPath);
    if (!jsonOpt) {
        std::cerr << "Failed to load aircraft config: " << configPath << std::endl;
        return;
    }

    const auto& json = *jsonOpt;
    JsbsimConfig jsbsimConfig;
    if (json.contains(ConfigKeys::JSBSIM)) {
        const auto& jsb = json[ConfigKeys::JSBSIM];
        jsbsimConfig.modelName = jsb.value(ConfigKeys::JSBSIM_MODEL, jsbsimConfig.modelName);
        jsbsimConfig.rootPath = jsb.value(ConfigKeys::JSBSIM_ROOT, jsbsimConfig.rootPath);
        jsbsimConfig.initLatDeg = jsb.value(ConfigKeys::JSBSIM_LAT, jsbsimConfig.initLatDeg);
        jsbsimConfig.initLonDeg = jsb.value(ConfigKeys::JSBSIM_LON, jsbsimConfig.initLonDeg);
    }

    // Initialize visuals
    m_visual.init(configPath, assets);

    addSystem<EnvironmentSystem>(atmosphere);
    addSystem<JsbsimSystem>(jsbsimConfig);

    Vec3 initialPos(0, 100, 0);
    double initialAirspeed = 0.0;
    if (json.contains(ConfigKeys::SPAWN)) {
        const auto& spawn = json[ConfigKeys::SPAWN];
        if (spawn.contains(ConfigKeys::POSITION)) {
            from_json(spawn[ConfigKeys::POSITION], initialPos);
        }
        initialAirspeed = spawn.value(ConfigKeys::AIRSPEED, 0.0);
    }
    
    m_currentState.position = initialPos;
    m_currentState.airspeed = initialAirspeed;
    m_currentState.orientation = Quat::identity();
    m_currentState.velocity = Vec3(0, 0, static_cast<float>(initialAirspeed));
    
    m_prevState = m_currentState;
}

void Aircraft::Instance::update(float dt) {
    m_prevState = m_currentState;

    // Read controls from global property tree
    double elevator = PropertyBus::global().get(Properties::Controls::ELEVATOR, 0.0);
    double aileron = PropertyBus::global().get(Properties::Controls::AILERON, 0.0);
    double rudder = PropertyBus::global().get(Properties::Controls::RUDDER, 0.0);
    double throttle = PropertyBus::global().get(Properties::Controls::THROTTLE, 0.0);

    m_state.set(Properties::Controls::ELEVATOR, elevator);
    m_state.set(Properties::Controls::AILERON, aileron);
    m_state.set(Properties::Controls::RUDDER, rudder);
    m_state.set(Properties::Controls::THROTTLE, throttle);

    for (auto& system : m_systems) {
        system->update(dt);
    }
}

void Aircraft::Instance::render(const Mat4& viewProjection, float alpha, const Vec3& lightDir) {
    Vec3 renderPos = interpolatedPosition(alpha);
    Quat renderRot = interpolatedOrientation(alpha);
    m_visual.draw(renderPos, renderRot, viewProjection, lightDir);
}

Vec3 Aircraft::Instance::interpolatedPosition(float alpha) const {
    return m_prevState.position + (m_currentState.position - m_prevState.position) * alpha;
}

Quat Aircraft::Instance::interpolatedOrientation(float alpha) const {
    return Quat::slerp(m_prevState.orientation, m_currentState.orientation, alpha);
}

}
