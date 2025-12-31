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
#include "math/geo.hpp"
#include "graphics/renderers/terrain_renderer.hpp"
#include "utils/config_loader.hpp"
#include "aircraft/aircraft_config_keys.hpp"

#include "aircraft/systems/physics/jsbsim_system.hpp"
#include "aircraft/systems/environment/environment_system.hpp"
#include <iostream>

namespace nuage {

void Aircraft::Instance::init(const std::string& configPath, AssetStore& assets, Atmosphere& atmosphere,
                              const GeoOrigin* terrainOrigin, const TerrainRenderer* terrain) {
    auto jsonOpt = loadJsonConfig(configPath);
    if (!jsonOpt) {
        std::cerr << "Failed to load aircraft config: " << configPath << std::endl;
        return;
    }

    m_properties.bind(PropertyBus::global(), m_state);

    const auto& json = *jsonOpt;
    Vec3 initialPos(0, 100, 0);
    double initialAirspeed = 0.0;
    if (json.contains(ConfigKeys::SPAWN)) {
        const auto& spawn = json[ConfigKeys::SPAWN];
        if (spawn.contains(ConfigKeys::POSITION)) {
            from_json(spawn[ConfigKeys::POSITION], initialPos);
        }
        initialAirspeed = spawn.value(ConfigKeys::AIRSPEED, 0.0);
    }
    // If terrain is available, snap the spawn altitude to the terrain height to avoid hovering.
    if (terrain) {
        float groundY = 0.0f;
        if (terrain->sampleSurfaceHeight(initialPos.x, initialPos.z, groundY)) {
            initialPos.y = groundY + 1.0f; // small clearance for wheels
        }
    }

    JsbsimConfig jsbsimConfig;
    if (json.contains(ConfigKeys::JSBSIM)) {
        const auto& jsb = json[ConfigKeys::JSBSIM];
        jsbsimConfig.modelName = jsb.value(ConfigKeys::JSBSIM_MODEL, jsbsimConfig.modelName);
        jsbsimConfig.rootPath = jsb.value(ConfigKeys::JSBSIM_ROOT, jsbsimConfig.rootPath);
        jsbsimConfig.initLatDeg = jsb.value(ConfigKeys::JSBSIM_LAT, jsbsimConfig.initLatDeg);
        jsbsimConfig.initLonDeg = jsb.value(ConfigKeys::JSBSIM_LON, jsbsimConfig.initLonDeg);
    }
    if (terrainOrigin) {
        jsbsimConfig.originLatDeg = terrainOrigin->latDeg;
        jsbsimConfig.originLonDeg = terrainOrigin->lonDeg;
        jsbsimConfig.originAltMeters = terrainOrigin->altMeters;
        jsbsimConfig.hasOrigin = true;
        // Align JSBSim initial lat/lon with terrain origin + spawn ENU offset.
        double spawnLat = terrainOrigin->latDeg;
        double spawnLon = terrainOrigin->lonDeg;
        double spawnAlt = terrainOrigin->altMeters;
        enuToLla(*terrainOrigin, initialPos, spawnLat, spawnLon, spawnAlt);
        jsbsimConfig.initLatDeg = spawnLat;
        jsbsimConfig.initLonDeg = spawnLon;
        // JSBSim takes altitude in feet; we set in ensureInitialized, but keep meters here.
    }
    if (terrain) {
        jsbsimConfig.terrain = terrain;
    }

    // Initialize visuals
    m_visual.init(configPath, assets);

    addSystem<EnvironmentSystem>(atmosphere);
    addSystem<JsbsimSystem>(jsbsimConfig);

    m_currentState.position = initialPos;
    m_currentState.airspeed = initialAirspeed;
    m_currentState.orientation = Quat::identity();
    m_currentState.velocity = Vec3(0, 0, static_cast<float>(initialAirspeed));
    
    m_prevState = m_currentState;
}

void Aircraft::Instance::update(float dt) {
    m_prevState = m_currentState;

    PropertyBus& global = m_properties.global();
    PropertyBus& local = m_properties.local();

    double elevator = global.get(Properties::Controls::ELEVATOR, 0.0);
    double aileron = global.get(Properties::Controls::AILERON, 0.0);
    double rudder = global.get(Properties::Controls::RUDDER, 0.0);
    double throttle = global.get(Properties::Controls::THROTTLE, 0.0);
    double flaps = global.get(Properties::Controls::FLAPS, 0.0);
    double brakeLeft = global.get(Properties::Controls::BRAKE_LEFT, 0.0);
    double brakeRight = global.get(Properties::Controls::BRAKE_RIGHT, 0.0);

    local.set(Properties::Controls::ELEVATOR, elevator);
    local.set(Properties::Controls::AILERON, aileron);
    local.set(Properties::Controls::RUDDER, rudder);
    local.set(Properties::Controls::THROTTLE, throttle);
    local.set(Properties::Controls::FLAPS, flaps);
    local.set(Properties::Controls::BRAKE_LEFT, brakeLeft);
    local.set(Properties::Controls::BRAKE_RIGHT, brakeRight);

    for (auto& system : m_systems) {
        system->update(dt);
    }
}

void Aircraft::Instance::applyGroundCollision(const TerrainRenderer& terrain) {
    if (auto jsb = getSystem<JsbsimSystem>()) {
        if (jsb->hasGroundCallback()) {
            return;
        }
    }
    float groundY = 0.0f;
    if (!terrain.sampleSurfaceHeight(m_currentState.position.x, m_currentState.position.z, groundY)) {
        return;
    }
    if (m_currentState.position.y < groundY) {
        m_currentState.position.y = groundY;
        if (m_currentState.velocity.y < 0.0f) {
            m_currentState.velocity.y = 0.0f;
        }
        if (m_prevState.position.y < groundY) {
            m_prevState.position.y = groundY;
        }
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
