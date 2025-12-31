#include "ui/overlays/hud_overlay.hpp"
#include "core/properties/property_paths.hpp"
#include "aircraft/aircraft.hpp"
#include <string>
#include <cmath>
#include <algorithm>

namespace nuage {

void HudOverlay::draw(UIManager& ui, Aircraft& aircraft) {
    Aircraft::Instance* player = aircraft.player();
    if (!player) return;

    const auto& bus = player->state();
    float airspeedKts = static_cast<float>(bus.get(Properties::Velocities::AIRSPEED_KT, 0.0));
    float altitudeFeet = static_cast<float>(bus.get(Properties::Position::ALTITUDE_FT, 0.0));
    float headingDegrees = static_cast<float>(bus.get(Properties::Orientation::HEADING_DEG, 0.0));
    float powerPercent = static_cast<float>(bus.get(Properties::Controls::THROTTLE, 0.0));
    float flapPercent = std::clamp(static_cast<float>(bus.get(Properties::Surfaces::FLAPS_NORM, 0.0)), 0.0f, 1.0f);
    float flapDeg = static_cast<float>(bus.get(Properties::Surfaces::FLAPS_DEG, 0.0));

    // Gauge constants
    constexpr float kHudLeftX = 20.0f;
    constexpr float kCompassSize = 230.0f;
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

    // 1. Throttle Gauge
    ui.drawRoundedRect(kGaugeX, kGaugeY, kGaugeWidth, kGaugeHeight, kGaugeRadius,
                       kGaugeOutline, 0.85f, Anchor::BottomLeft);
    ui.drawRoundedRect(kGaugeX + kInset, kGaugeY + kInset,
                       kGaugeWidth - 2.0f * kInset, kGaugeHeight - 2.0f * kInset,
                       kGaugeRadius - kInset, kGaugeBack, 0.85f, Anchor::BottomLeft);

    float percent = std::clamp(powerPercent, 0.0f, 1.0f);
    float innerHeight = (kGaugeHeight - 2.0f * kInset) * percent;
    if (innerHeight > 0.0f) {
        ui.drawRoundedRect(kGaugeX + kInset, kGaugeY + kInset,
                           kGaugeWidth - 2.0f * kInset, innerHeight,
                           kGaugeRadius - kInset, kGaugeFill, 0.9f, Anchor::BottomLeft);
    }

    int percentValue = static_cast<int>(std::round(percent * 100.0f));
    std::string percentText = std::to_string(percentValue) + "%";
    ui.drawText(percentText, kGaugeX, -(kGaugeY + kGaugeHeight + 8.0f),
                Anchor::BottomLeft, 0.55f, kGaugeSubText, 0.9f);

    // 2. Compass
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

    float heading = headingDegrees;
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

    int headVal = static_cast<int>(std::round(heading)) % 360;
    if (headVal < 0) headVal += 360;
    char headBuf[4];
    snprintf(headBuf, sizeof(headBuf), "%03d", headVal);
    ui.drawText(headBuf,
                kCompassX + kCompassRadius - centerOffsetX,
                kCompassY + kCompassRadius - centerOffsetY,
                Anchor::Center, 0.75f, kCompassValue, 0.98f);

    // 3. Info Box (Alt/Speed)
    constexpr float kInfoBoxPadding = 16.0f;
    constexpr float kInfoBoxHeight = 96.0f;
    constexpr float kInfoBoxRadius = 10.0f;
    const Vec3 kInfoBoxBack = Vec3(0.18f, 0.2f, 0.23f);
    const Vec3 kInfoText = Vec3(0.95f, 0.96f, 0.98f);

    auto formatWithCommas = [](int value) {
        std::string s = std::to_string(value);
        int insertPos = static_cast<int>(s.size()) - 3;
        while (insertPos > 0) {
            s.insert(static_cast<std::string::size_type>(insertPos), ",");
            insertPos -= 3;
        }
        return s;
    };

    std::string altText = formatWithCommas(static_cast<int>(std::round(altitudeFeet))) + " ft";
    std::string speedText = formatWithCommas(static_cast<int>(std::round(airspeedKts))) + " kts";
    int flapDegInt = static_cast<int>(std::round(flapDeg));
    int flapPercentInt = static_cast<int>(std::round(flapPercent * 100.0f));
    std::string flapText = "Flaps " + std::to_string(flapDegInt) + " deg (" + std::to_string(flapPercentInt) + "%)";

    float infoBoxX = kCompassX;
    float infoBoxY = kCompassY + kCompassSize + kInfoBoxPadding;
    float infoBoxW = kCompassSize;

    ui.drawRoundedRect(infoBoxX, infoBoxY, infoBoxW, kInfoBoxHeight, kInfoBoxRadius,
                       kInfoBoxBack, 0.92f, Anchor::TopLeft);
    constexpr float kInfoTextPadX = 16.0f;
    constexpr float kInfoTextPadTop = 12.0f;
    constexpr float kInfoLineGap = 26.0f;
    ui.drawText(altText, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop,
                Anchor::TopLeft, 0.55f, kInfoText, 0.98f);
    ui.drawText(speedText, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop + kInfoLineGap,
                Anchor::TopLeft, 0.55f, kInfoText, 0.98f);
    ui.drawText(flapText, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop + kInfoLineGap * 2.0f,
                Anchor::TopLeft, 0.55f, kInfoText, 0.98f);
}

}
