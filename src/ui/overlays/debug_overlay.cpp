#include "ui/overlays/debug_overlay.hpp"
#include "core/app.hpp"
#include "core/session/flight_session.hpp"
#include "graphics/renderers/terrain_renderer.hpp"
#include "ui/button.hpp"
#include <algorithm>
#include <cstdio>

namespace nuage {

namespace {
constexpr float kPanelWidth = 384.0f;
constexpr float kPanelHeight = 264.0f;
constexpr float kPanelRadius = 18.0f;
constexpr float kPanelMargin = 28.0f;
constexpr float kRowHeight = 38.0f;
constexpr float kHeaderHeight = 52.0f;
constexpr float kPaddingX = 20.0f;
constexpr float kHeaderTextY = 14.0f;
constexpr float kDividerY = 40.0f;
constexpr float kButtonSize = 24.0f;
constexpr float kButtonGap = 10.0f;
constexpr float kValueOffset = 176.0f;

const Vec3 kPanelColor = Vec3(0.08f, 0.1f, 0.14f);
const Vec3 kPanelOutline = Vec3(0.28f, 0.32f, 0.38f);
const Vec3 kDividerColor = Vec3(0.2f, 0.23f, 0.28f);
const Vec3 kButtonFill = Vec3(0.14f, 0.18f, 0.24f);
const Vec3 kButtonHover = Vec3(0.22f, 0.28f, 0.36f);
const Vec3 kTextMain = Vec3(0.95f, 0.96f, 0.98f);
const Vec3 kTextSub = Vec3(0.72f, 0.76f, 0.82f);

constexpr float kFogStep = 500.0f;
constexpr float kNoiseStep = 0.05f;
} // namespace

void DebugOverlay::update(bool active, UIManager& ui) {
    if (!m_initialized) {
        buildUi(ui);
    }

    layout(ui);

    if (m_visible != active) {
        setButtonsVisible(active);
        m_visible = active;
    }
}

void DebugOverlay::draw(bool active, UIManager& ui) {
    if (!active) return;

    ui.drawRoundedRect(m_panelX, m_panelY, kPanelWidth, kPanelHeight, kPanelRadius,
                       kPanelColor, 0.92f, Anchor::TopLeft);
    ui.drawRoundedRect(m_panelX, m_panelY, kPanelWidth, kPanelHeight, kPanelRadius,
                       kPanelOutline, 0.4f, Anchor::TopLeft);

    ui.drawText("DEBUG TERRAIN", m_panelX + kPaddingX, m_panelY + kHeaderTextY,
                Anchor::TopLeft, 0.6f, kTextMain, 0.95f);
    ui.drawRect(m_panelX + kPaddingX, m_panelY + kDividerY,
                kPanelWidth - 2.0f * kPaddingX, 1.5f, kDividerColor, 0.7f, Anchor::TopLeft);

    App* app = ui.app();
    FlightSession* session = app ? app->session() : nullptr;
    TerrainRenderer* terrain = session ? &session->terrain() : nullptr;

    if (!terrain) {
        ui.drawText("No active terrain", m_panelX + kPaddingX, m_panelY + 60.0f,
                    Anchor::TopLeft, 0.5f, kTextSub, 0.9f);
        return;
    }

    const bool compiled = terrain->isCompiled();
    const bool procedural = terrain->isProcedural();
    const char* mode = compiled ? "Compiled" : (procedural ? "Procedural" : "None");
    float modeX = m_panelX + kPanelWidth - kPaddingX - 110.0f;
    ui.drawText(mode, modeX, m_panelY + kHeaderTextY + 2.0f,
                Anchor::TopLeft, 0.5f, kTextSub, 0.9f);

    int row = 0;
    auto rowY = [&](int index) {
        return m_panelY + kHeaderHeight + index * kRowHeight;
    };

    auto drawRow = [&](const char* label, const std::string& value, int index) {
        float y = rowY(index) + 7.0f;
        ui.drawText(label, m_panelX + kPaddingX, y, Anchor::TopLeft, 0.5f, kTextMain, 0.95f);
        ui.drawText(value, m_panelX + kPaddingX + kValueOffset, y, Anchor::TopLeft, 0.5f, kTextSub, 0.95f);
    };

    int visibleRadius = compiled ? terrain->compiledVisibleRadius() : terrain->proceduralVisibleRadius();
    int loadsPerFrame = compiled ? terrain->compiledLoadsPerFrame() : terrain->proceduralLoadsPerFrame();

    drawRow("Visible Radius", std::to_string(visibleRadius), row++);
    drawRow("Loads / Frame", std::to_string(loadsPerFrame), row++);

    char fogBuffer[32];
    std::snprintf(fogBuffer, sizeof(fogBuffer), "%.0f m", terrain->visuals().fogDistance);
    drawRow("Fog Distance", fogBuffer, row++);

    char noiseBuffer[32];
    std::snprintf(noiseBuffer, sizeof(noiseBuffer), "%.2f", terrain->visuals().noiseStrength);
    drawRow("Noise Strength", noiseBuffer, row++);

}

void DebugOverlay::reset() {
    setButtonsVisible(false);
    m_visible = false;
}

void DebugOverlay::buildUi(UIManager& ui) {
    if (m_initialized) return;

    auto makeButton = [&](const std::string& label) -> Button* {
        Button& button = ui.button(label);
        button.size(kButtonSize, kButtonSize)
            .anchorMode(Anchor::TopLeft)
            .colorR(kButtonFill.x, kButtonFill.y, kButtonFill.z)
            .hoverColor(kButtonHover)
            .cornerRadius(6.0f)
            .scaleVal(0.7f);
        return &button;
    };

    m_radiusMinus = makeButton("-");
    m_radiusPlus = makeButton("+");
    m_loadsMinus = makeButton("-");
    m_loadsPlus = makeButton("+");
    m_fogMinus = makeButton("-");
    m_fogPlus = makeButton("+");
    m_noiseMinus = makeButton("-");
    m_noisePlus = makeButton("+");

    App* app = ui.app();

    auto getTerrain = [app]() -> TerrainRenderer* {
        if (!app) return nullptr;
        FlightSession* session = app->session();
        return session ? &session->terrain() : nullptr;
    };

    if (m_radiusMinus) {
        m_radiusMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            if (terrain->isCompiled()) {
                terrain->setCompiledVisibleRadius(terrain->compiledVisibleRadius() - 1);
            } else if (terrain->isProcedural()) {
                terrain->setProceduralVisibleRadius(terrain->proceduralVisibleRadius() - 1);
            }
        });
    }
    if (m_radiusPlus) {
        m_radiusPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            if (terrain->isCompiled()) {
                terrain->setCompiledVisibleRadius(terrain->compiledVisibleRadius() + 1);
            } else if (terrain->isProcedural()) {
                terrain->setProceduralVisibleRadius(terrain->proceduralVisibleRadius() + 1);
            }
        });
    }
    if (m_loadsMinus) {
        m_loadsMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            if (terrain->isCompiled()) {
                terrain->setCompiledLoadsPerFrame(terrain->compiledLoadsPerFrame() - 1);
            } else if (terrain->isProcedural()) {
                terrain->setProceduralLoadsPerFrame(terrain->proceduralLoadsPerFrame() - 1);
            }
        });
    }
    if (m_loadsPlus) {
        m_loadsPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            if (terrain->isCompiled()) {
                terrain->setCompiledLoadsPerFrame(terrain->compiledLoadsPerFrame() + 1);
            } else if (terrain->isProcedural()) {
                terrain->setProceduralLoadsPerFrame(terrain->proceduralLoadsPerFrame() + 1);
            }
        });
    }
    if (m_fogMinus) {
        m_fogMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            float next = std::max(500.0f, terrain->visuals().fogDistance - kFogStep);
            terrain->visuals().fogDistance = next;
            terrain->clampVisuals();
        });
    }
    if (m_fogPlus) {
        m_fogPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            terrain->visuals().fogDistance += kFogStep;
            terrain->clampVisuals();
        });
    }
    if (m_noiseMinus) {
        m_noiseMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            float next = std::clamp(terrain->visuals().noiseStrength - kNoiseStep, 0.0f, 1.0f);
            terrain->visuals().noiseStrength = next;
            terrain->clampVisuals();
        });
    }
    if (m_noisePlus) {
        m_noisePlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            float next = std::clamp(terrain->visuals().noiseStrength + kNoiseStep, 0.0f, 1.0f);
            terrain->visuals().noiseStrength = next;
            terrain->clampVisuals();
        });
    }

    m_initialized = true;
    setButtonsVisible(false);
}

void DebugOverlay::layout(UIManager& ui) {
    int width = ui.getWindowWidth();
    int height = ui.getWindowHeight();
    if (width == m_lastWidth && height == m_lastHeight) return;

    m_lastWidth = width;
    m_lastHeight = height;

    m_panelX = static_cast<float>(width) - kPanelMargin - kPanelWidth;
    m_panelY = kPanelMargin;

    auto positionRowButtons = [&](Button* minus, Button* plus, int index) {
        if (!minus || !plus) return;
        float rowY = m_panelY + kHeaderHeight + index * kRowHeight;
        float buttonY = rowY + (kRowHeight - kButtonSize) * 0.5f;
        float rightEdge = m_panelX + kPanelWidth - kPaddingX;
        float plusX = rightEdge - kButtonSize;
        float minusX = plusX - kButtonSize - kButtonGap;
        minus->pos(minusX, buttonY);
        plus->pos(plusX, buttonY);
    };

    positionRowButtons(m_radiusMinus, m_radiusPlus, 0);
    positionRowButtons(m_loadsMinus, m_loadsPlus, 1);
    positionRowButtons(m_fogMinus, m_fogPlus, 2);
    positionRowButtons(m_noiseMinus, m_noisePlus, 3);
}

void DebugOverlay::setButtonsVisible(bool visible) {
    if (m_radiusMinus) m_radiusMinus->setVisible(visible).setEnabled(visible);
    if (m_radiusPlus) m_radiusPlus->setVisible(visible).setEnabled(visible);
    if (m_loadsMinus) m_loadsMinus->setVisible(visible).setEnabled(visible);
    if (m_loadsPlus) m_loadsPlus->setVisible(visible).setEnabled(visible);
    if (m_fogMinus) m_fogMinus->setVisible(visible).setEnabled(visible);
    if (m_fogPlus) m_fogPlus->setVisible(visible).setEnabled(visible);
    if (m_noiseMinus) m_noiseMinus->setVisible(visible).setEnabled(visible);
    if (m_noisePlus) m_noisePlus->setVisible(visible).setEnabled(visible);
}

} // namespace nuage
