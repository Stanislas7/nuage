#include "ui/overlays/pause_overlay.hpp"
#include "core/app.hpp"
#include "core/session/flight_session.hpp"
#include "environment/atmosphere.hpp"
#include "ui/button.hpp"
#include "math/vec3.hpp"
#include <cstdio>

namespace nuage {

namespace {
constexpr float kPanelWidth = 640.0f;
constexpr float kPanelHeight = 260.0f;
constexpr float kPanelRadius = 18.0f;
constexpr float kPanelBottomMargin = 80.0f;
constexpr float kButtonWidth = 220.0f;
constexpr float kButtonHeight = 46.0f;
constexpr float kButtonSpacing = 32.0f;
constexpr float kRowGap = 26.0f;
constexpr float kRow1Offset = 28.0f;
constexpr float kTitleOffset = -96.0f;
constexpr float kCurrentOffset = -56.0f;
const Vec3 kPanelColor = Vec3(0.08f, 0.1f, 0.14f);
const Vec3 kPanelHoverColor = Vec3(0.11f, 0.13f, 0.18f);
const Vec3 kButtonBlue = Vec3(0.12f, 0.45f, 0.86f);
const Vec3 kButtonBlueHover = Vec3(0.18f, 0.54f, 0.94f);
const Vec3 kOutlineLight = Vec3(0.75f, 0.78f, 0.84f);
}

void PauseOverlay::update(bool paused, UIManager& ui) {
    if (!m_initialized) {
        buildUi(ui);
    }

    layout(ui);

    if (m_visible != paused) {
        setButtonsVisible(paused);
        m_visible = paused;
    }
}

void PauseOverlay::draw(bool paused, UIManager& ui) {
    if (!paused) return;

    // Draw background tint
    ui.drawRect(0, 0, (float)ui.getWindowWidth(), (float)ui.getWindowHeight(),
                Vec3(0.0f, 0.0f, 0.0f), 0.45f);

    // Draw text
    ui.drawText("PAUSED", 0, -220, Anchor::Center, 1.5f, Vec3(1, 1, 1), 1.0f);

    ui.drawText("Time of Day Presets", 0, m_panelCenterY + kTitleOffset, Anchor::Center, 0.6f, Vec3(1, 1, 1), 1.0f);

    App* app = ui.app();
    if (app && app->session()) {
        char buffer[32];
        float hours = app->session()->atmosphere().getTimeOfDay();
        std::snprintf(buffer, sizeof(buffer), "Current: %.1f h", hours);
        ui.drawText(buffer, 0, m_panelCenterY + kCurrentOffset, Anchor::Center, 0.5f, Vec3(0.85f, 0.85f, 0.85f), 1.0f);
    }
}

void PauseOverlay::reset() {
    setButtonsVisible(false);
    m_visible = false;
}

void PauseOverlay::buildUi(UIManager& ui) {
    if (m_initialized) return;

    auto makeButton = [&](const std::string& label, float x, float y, float hour) -> Button* {
        Button& button = ui.button(label);
        button.size(kButtonWidth, kButtonHeight)
            .pos(x, y)
            .anchorMode(Anchor::Center)
            .colorR(kPanelColor.x, kPanelColor.y, kPanelColor.z)
            .hoverColor(kPanelHoverColor)
            .cornerRadius(14.0f)
            .outlineOnly(true)
            .outlineColor(kOutlineLight)
            .outlineThickness(2.0f)
            .scaleVal(0.55f);

        App* app = ui.app();
        button.onClick([app, hour]() {
            if (!app) return;
            FlightSession* session = app->session();
            if (!session) return;
            session->atmosphere().setTimeOfDay(hour);
        });

        return &button;
    };

    m_dawnButton = makeButton("Dawn 06:00", 0.0f, 0.0f, 6.0f);
    m_noonButton = makeButton("Noon 12:00", 0.0f, 0.0f, 12.0f);
    m_duskButton = makeButton("Dusk 18:00", 0.0f, 0.0f, 18.0f);
    m_midnightButton = makeButton("Midnight 00:00", 0.0f, 0.0f, 0.0f);

    Button& resume = ui.button("RESUME");
    resume.size(380.0f, 76.0f)
        .pos(36.0f, 36.0f)
        .anchorMode(Anchor::BottomRight)
        .colorR(kButtonBlue.x, kButtonBlue.y, kButtonBlue.z)
        .hoverColor(kButtonBlueHover)
        .cornerRadius(22.0f)
        .scaleVal(0.8f);

    App* app = ui.app();
    resume.onClick([app]() {
        if (!app) return;
        app->setPaused(false);
    });
    m_resumeButton = &resume;

    m_initialized = true;
    setButtonsVisible(false);
}

void PauseOverlay::layout(UIManager& ui) {
    int width = ui.getWindowWidth();
    int height = ui.getWindowHeight();
    if (width == m_lastWidth && height == m_lastHeight) return;

    m_lastWidth = width;
    m_lastHeight = height;

    m_panelCenterY = (height * 0.5f) - kPanelBottomMargin - (kPanelHeight * 0.5f);

    float xOffset = (kButtonWidth * 0.5f) + (kButtonSpacing * 0.5f);
    float row1Y = m_panelCenterY + kRow1Offset;
    float row2Y = row1Y + kButtonHeight + kRowGap;

    if (m_dawnButton) m_dawnButton->pos(-xOffset, row1Y);
    if (m_noonButton) m_noonButton->pos(xOffset, row1Y);
    if (m_duskButton) m_duskButton->pos(-xOffset, row2Y);
    if (m_midnightButton) m_midnightButton->pos(xOffset, row2Y);
}

void PauseOverlay::setButtonsVisible(bool visible) {
    if (m_dawnButton) m_dawnButton->setVisible(visible).setEnabled(visible);
    if (m_noonButton) m_noonButton->setVisible(visible).setEnabled(visible);
    if (m_duskButton) m_duskButton->setVisible(visible).setEnabled(visible);
    if (m_midnightButton) m_midnightButton->setVisible(visible).setEnabled(visible);
    if (m_resumeButton) m_resumeButton->setVisible(visible).setEnabled(visible);
}

} // namespace nuage
