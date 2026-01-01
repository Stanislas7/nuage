#include "ui/overlays/hud_overlay.hpp"
#include "core/properties/property_paths.hpp"
#include "aircraft/aircraft.hpp"
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace nuage {

void HudOverlay::draw(UIManager& ui, Aircraft& aircraft) {
    Aircraft::Instance* player = aircraft.player();
    if (!player) return;

    const auto& bus = player->state();
    float airspeedKts = static_cast<float>(bus.get(Properties::Velocities::AIRSPEED_KT, 0.0));
    float airspeedIasKts = static_cast<float>(bus.get(Properties::Velocities::AIRSPEED_IAS_KT, 0.0));
    float groundSpeedKts = static_cast<float>(bus.get(Properties::Velocities::GROUND_SPEED_KT, 0.0));
    float altitudeFeet = static_cast<float>(bus.get(Properties::Position::ALTITUDE_FT, 0.0));
    float altitudeAglFeet = static_cast<float>(bus.get(Properties::Position::ALTITUDE_AGL_FT, 0.0));
    float headingDegrees = static_cast<float>(bus.get(Properties::Orientation::HEADING_DEG, 0.0));
    float powerPercent = static_cast<float>(bus.get(Properties::Controls::THROTTLE, 0.0));
    float flapPercent = std::clamp(static_cast<float>(bus.get(Properties::Surfaces::FLAPS_NORM, 0.0)), 0.0f, 1.0f);
    float flapDeg = static_cast<float>(bus.get(Properties::Surfaces::FLAPS_DEG, 0.0));

    // Gauge constants
    constexpr float kHudLeftX = 20.0f;
    constexpr float kCompassSize = 280.0f;
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

    // Throttle Gauge
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

    // Info Box (Alt/Speed)
    constexpr float kInfoBoxPadding = 16.0f;
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

    std::string altText = "ALT MSL " + formatWithCommas(static_cast<int>(std::round(altitudeFeet))) + " ft";
    std::string aglText = "ALT AGL " + formatWithCommas(static_cast<int>(std::round(altitudeAglFeet))) + " ft";
    std::string speedText = "TAS " + formatWithCommas(static_cast<int>(std::round(airspeedKts))) + " kts";
    std::string iasGsText = "IAS " + formatWithCommas(static_cast<int>(std::round(airspeedIasKts)))
        + " kts  GS " + formatWithCommas(static_cast<int>(std::round(groundSpeedKts))) + " kts";
    int flapDegInt = static_cast<int>(std::round(flapDeg));
    int flapPercentInt = static_cast<int>(std::round(flapPercent * 100.0f));
    std::string flapText = "Flaps " + std::to_string(flapDegInt) + " deg (" + std::to_string(flapPercentInt) + "%)";
    int headVal = static_cast<int>(std::round(headingDegrees)) % 360;
    if (headVal < 0) headVal += 360;
    char headBuf[32];
    std::snprintf(headBuf, sizeof(headBuf), "Heading %03d deg", headVal);

    float infoBoxX = kHudLeftX;
    float infoBoxY = kHudLeftX + kInfoBoxPadding;
    float infoBoxW = kCompassSize;
    constexpr float kInfoTextPadX = 16.0f;
    constexpr float kInfoTextPadTop = 12.0f;
    constexpr int kInfoLineCount = 6;
    constexpr float kInfoLineGap = 26.0f;
    constexpr float kInfoTextScale = 0.55f;
    Vec3 lineSize = ui.measureText("Ag", kInfoTextScale);
    float lineHeight = lineSize.y > 0.0f ? lineSize.y : 18.0f;
    constexpr float kInfoTextPadBottom = 12.0f;
    float infoBoxHeight = kInfoTextPadTop + lineHeight + (kInfoLineCount - 1) * kInfoLineGap + kInfoTextPadBottom;

    ui.drawRoundedRect(infoBoxX, infoBoxY, infoBoxW, infoBoxHeight, kInfoBoxRadius,
                       kInfoBoxBack, 0.92f, Anchor::TopLeft);
    ui.drawText(altText, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop,
                Anchor::TopLeft, kInfoTextScale, kInfoText, 0.98f);
    ui.drawText(aglText, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop + kInfoLineGap,
                Anchor::TopLeft, kInfoTextScale, kInfoText, 0.98f);
    ui.drawText(speedText, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop + kInfoLineGap * 2.0f,
                Anchor::TopLeft, kInfoTextScale, kInfoText, 0.98f);
    ui.drawText(iasGsText, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop + kInfoLineGap * 3.0f,
                Anchor::TopLeft, kInfoTextScale, kInfoText, 0.98f);
    ui.drawText(flapText, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop + kInfoLineGap * 4.0f,
                Anchor::TopLeft, kInfoTextScale, kInfoText, 0.98f);
    ui.drawText(headBuf, infoBoxX + kInfoTextPadX, infoBoxY + kInfoTextPadTop + kInfoLineGap * 5.0f,
                Anchor::TopLeft, kInfoTextScale, kInfoText, 0.98f);
}

}
