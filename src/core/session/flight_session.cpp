#include "core/session/flight_session.hpp"
#include "core/app.hpp"
#include "core/properties/property_paths.hpp"
#include "graphics/glad.h"
#include <algorithm>
#include <cmath>

namespace nuage {

namespace {
constexpr float kHudLeftX = 20.0f;
constexpr float kCompassSize = 230.0f;
}

FlightSession::FlightSession(App* app, const FlightConfig& config)
    : m_app(app), m_config(config) {
}

bool FlightSession::init() {
    auto assets = m_app->subsystems().getRequired<AssetStore>();

    m_atmosphere.init();
    m_atmosphere.setTimeOfDay(m_config.timeOfDay);
    
    m_aircraft.init(*assets, m_atmosphere);
    m_camera.init(m_app->input());
    m_skybox.init(*assets);
    m_terrain.init(*assets);
    m_terrain.setup(m_config.terrainPath, *assets);

    if (!m_config.aircraftPath.empty()) {
        if (m_terrain.hasCompiledOrigin()) {
            GeoOrigin origin = m_terrain.compiledOrigin();
            m_aircraft.spawnPlayer(m_config.aircraftPath, &origin, &m_terrain);
        } else {
            m_aircraft.spawnPlayer(m_config.aircraftPath, nullptr, &m_terrain);
        }
    }

    return true;
}

void FlightSession::shutdown() {
    m_aircraft.shutdown();
    m_terrain.shutdown();
    m_skybox.shutdown();
}

void FlightSession::update(float dt) {
    m_atmosphere.update(dt);
}

void FlightSession::render(float alpha) {
    Mat4 view = m_camera.viewMatrix();
    Mat4 proj = m_camera.projectionMatrix();
    Mat4 vp = proj * view;

    float simTime = static_cast<float>(PropertyBus::global().get(Properties::Sim::TIME, 0.0));
    m_skybox.render(view, proj, m_atmosphere, simTime);
    Vec3 sunDir = m_atmosphere.getSunDirection();
    m_terrain.render(vp, sunDir, m_camera.position());
    
    m_aircraft.render(vp, alpha, sunDir);
}

} // namespace nuage
