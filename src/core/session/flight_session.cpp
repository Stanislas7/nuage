#include "core/session/flight_session.hpp"
#include "core/app.hpp"
#include "core/properties/property_paths.hpp"
#include "graphics/glad.h"
#include <algorithm>
#include <iostream>
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
    AssetStore& assets = m_app->assets();

    m_atmosphere.init();
    m_atmosphere.setTimeOfDay(m_config.timeOfDay);
    
    m_aircraft.init(assets, m_atmosphere);
    m_camera.init(m_app->input());
    m_skybox.init(assets);
    m_terrain.init(assets);
    m_terrain.setup(m_config.terrainPath, assets);

    setupHUD();

    if (!m_config.aircraftPath.empty()) {
        m_aircraft.spawnPlayer(m_config.aircraftPath);
    }

    return true;
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

void FlightSession::setupHUD() {
}

void FlightSession::drawHUD(UIManager& ui) {
    m_hud.draw(ui, m_aircraft);
}

} // namespace nuage
