#include "core/session/flight_session.hpp"
#include "core/app.hpp"
#include "core/properties/property_paths.hpp"
#include "graphics/glad.h"
#include "ui/text.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace nuage {

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
    m_elapsedTime += dt;
    m_atmosphere.update(dt);
    updateHUD();
}

void FlightSession::render(float alpha) {
    Mat4 view = m_camera.viewMatrix();
    Mat4 proj = m_camera.projectionMatrix();
    Mat4 vp = proj * view;

    m_skybox.render(view, proj, m_atmosphere, m_elapsedTime);
    Vec3 sunDir = m_atmosphere.getSunDirection();
    m_terrain.render(vp, sunDir, m_camera.position());
    
    m_aircraft.render(vp, alpha, sunDir);
}

void FlightSession::setupHUD() {
    UIManager& ui = m_app->ui();
    auto setupText = [](Text& text, float x, float y, float scale, Anchor anchor, float padding) {
        text.scaleVal(scale);
        text.pos(x, y);
        text.colorR(1, 1, 1);
        text.anchorMode(anchor);
        text.paddingValue(padding);
    };

    m_altitudeText = &ui.text("ALT: 0.0 ft");
    setupText(*m_altitudeText, 20, 20, 0.6f, Anchor::TopLeft, 0.0f);

    m_airspeedText = &ui.text("SPD: 0 kts");
    setupText(*m_airspeedText, 20, 70, 0.6f, Anchor::TopLeft, 0.0f);

    m_headingText = &ui.text("HDG: 000");
    setupText(*m_headingText, 20, 120, 0.6f, Anchor::TopLeft, 0.0f);

    m_positionText = &ui.text("POS: 0, 0, 0");
    setupText(*m_positionText, 20, 170, 0.6f, Anchor::TopLeft, 0.0f);

}

void FlightSession::updateHUD() {
    Aircraft::Instance* player = m_aircraft.player();
    if (player && m_altitudeText && m_airspeedText && m_headingText && m_positionText) {
        Vec3 pos = player->position();
        float airspeed = player->airspeed();
        Vec3 fwd = player->forward();

        float heading = std::atan2(fwd.x, fwd.z) * 180.0f / 3.14159265f;
        if (heading < 0) heading += 360.0f;

        float airspeedKnots = airspeed * 1.94384f;
        auto fmt = [](float val) {
            std::string s = std::to_string(val);
            return s.substr(0, s.find('.') + 2);
        };

        m_altitudeText->content("ALT: " + fmt(pos.y * 3.28084f) + " ft");
        m_airspeedText->content("SPD: " + fmt(airspeedKnots) + " kts");
        m_headingText->content("HDG: " + std::to_string(static_cast<int>(heading)));
        m_positionText->content("POS: " + std::to_string(static_cast<int>(pos.x)) + ", " +
                                std::to_string(static_cast<int>(pos.y)) + ", " +
                                std::to_string(static_cast<int>(pos.z)));
    }

    if (player) {
        double throttle = player->state().get(Properties::Input::THROTTLE);
        m_powerPercent = static_cast<float>(std::clamp(throttle, 0.0, 1.0));
    } else {
        m_powerPercent = -1.0f;
    }

}

void FlightSession::drawHUD(UIManager& ui) {
    constexpr float kGaugeX = 24.0f;
    constexpr float kGaugeY = 48.0f;
    constexpr float kGaugeWidth = 56.0f;
    constexpr float kGaugeHeight = 220.0f;
    constexpr float kGaugeRadius = 12.0f;
    constexpr float kInset = 5.0f;

    const Vec3 kGaugeOutline = Vec3(0.62f, 0.86f, 0.7f);
    const Vec3 kGaugeBack = Vec3(0.07f, 0.12f, 0.1f);
    const Vec3 kGaugeFill = Vec3(0.06f, 0.78f, 0.28f);
    const Vec3 kGaugeText = Vec3(1.0f, 1.0f, 1.0f);
    const Vec3 kGaugeSubText = Vec3(1.0f, 1.0f, 1.0f);

    ui.drawRoundedRect(kGaugeX, kGaugeY, kGaugeWidth, kGaugeHeight, kGaugeRadius,
                       kGaugeOutline, 0.85f, Anchor::BottomLeft);
    ui.drawRoundedRect(kGaugeX + kInset, kGaugeY + kInset,
                       kGaugeWidth - 2.0f * kInset, kGaugeHeight - 2.0f * kInset,
                       kGaugeRadius - kInset, kGaugeBack, 0.85f, Anchor::BottomLeft);

    float percent = std::clamp(m_powerPercent, 0.0f, 1.0f);
    float innerHeight = (kGaugeHeight - 2.0f * kInset) * percent;
    if (m_powerPercent >= 0.0f && innerHeight > 0.0f) {
        ui.drawRoundedRect(kGaugeX + kInset, kGaugeY + kInset,
                           kGaugeWidth - 2.0f * kInset, innerHeight,
                           kGaugeRadius - kInset, kGaugeFill, 0.9f, Anchor::BottomLeft);
    }

    std::string percentText = "--";
    if (m_powerPercent >= 0.0f) {
        int percentValue = static_cast<int>(std::round(percent * 100.0f));
        percentText = std::to_string(percentValue) + "%";
    }
    ui.drawText(percentText, kGaugeX, -(kGaugeY + kGaugeHeight + 8.0f),
                Anchor::BottomLeft, 0.55f, kGaugeSubText, 0.9f);

}

} // namespace nuage
