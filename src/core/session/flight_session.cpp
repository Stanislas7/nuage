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
    (void)ui;
}

void FlightSession::updateHUD() {
    Aircraft::Instance* player = m_aircraft.player();
    if (player) {
        Vec3 fwd = player->forward();

        float heading = std::atan2(fwd.x, fwd.z) * 180.0f / 3.14159265f;
        if (heading < 0) heading += 360.0f;

        m_headingDegrees = heading;
    }

    if (player) {
        double throttle = player->state().get(Properties::Input::THROTTLE);
        m_powerPercent = static_cast<float>(std::clamp(throttle, 0.0, 1.0));
    } else {
        m_powerPercent = -1.0f;
        m_headingDegrees = -1.0f;
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

    constexpr float kCompassX = kHudLeftX;
    constexpr float kCompassY = kHudLeftX;
    constexpr float kCompassRadius = kCompassSize * 0.5f;
    constexpr float kCompassInset = 4.0f;

    const Vec3 kCompassOutline = Vec3(0.78f, 0.86f, 0.9f);
    const Vec3 kCompassBack = Vec3(0.06f, 0.08f, 0.1f);
    const Vec3 kCompassText = Vec3(0.92f, 0.95f, 0.98f);
    const Vec3 kCompassValue = Vec3(0.35f, 0.92f, 0.6f);

    ui.drawRoundedRect(kCompassX, kCompassY, kCompassSize, kCompassSize, kCompassRadius,
                       kCompassOutline, 0.9f, Anchor::TopLeft);
    ui.drawRoundedRect(kCompassX + kCompassInset, kCompassY + kCompassInset,
                       kCompassSize - 2.0f * kCompassInset, kCompassSize - 2.0f * kCompassInset,
                       kCompassRadius - kCompassInset, kCompassBack, 0.92f, Anchor::TopLeft);

    float heading = (m_headingDegrees >= 0.0f) ? m_headingDegrees : 0.0f;
    constexpr float kLetterPadding = 20.0f;
    float dialRadius = kCompassRadius - kLetterPadding;
    float centerOffsetX = static_cast<float>(ui.getWindowWidth()) * 0.5f;
    float centerOffsetY = static_cast<float>(ui.getWindowHeight()) * 0.5f;
    auto placeDir = [&](const char* label, float baseDeg) {
        float angle = (baseDeg - heading) * 3.14159265f / 180.0f;
        float x = kCompassX + kCompassRadius + std::sin(angle) * dialRadius;
        float y = kCompassY + kCompassRadius - std::cos(angle) * dialRadius;
        ui.drawText(label, x - centerOffsetX, y - centerOffsetY, Anchor::Center, 0.8f, kCompassText, 0.98f);
    };
    placeDir("N", 0.0f);
    placeDir("E", 90.0f);
    placeDir("S", 180.0f);
    placeDir("W", 270.0f);

    std::string headingText = "---";
    if (m_headingDegrees >= 0.0f) {
        int headingValue = static_cast<int>(std::round(m_headingDegrees)) % 360;
        if (headingValue < 0) headingValue += 360;
        if (headingValue < 10) {
            headingText = "00" + std::to_string(headingValue);
        } else if (headingValue < 100) {
            headingText = "0" + std::to_string(headingValue);
        } else {
            headingText = std::to_string(headingValue);
        }
    }
    ui.drawText(headingText,
                kCompassX + kCompassRadius - centerOffsetX,
                kCompassY + kCompassRadius - centerOffsetY,
                Anchor::Center, 0.75f, kCompassValue, 0.98f);
}

} // namespace nuage
