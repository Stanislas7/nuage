#include "ui/overlays/debug_overlay.hpp"
#include "core/app.hpp"
#include "core/properties/property_bus.hpp"
#include "core/properties/property_paths.hpp"
#include "core/session/flight_session.hpp"
#include "graphics/renderers/terrain_renderer.hpp"
#include "ui/button.hpp"
#include <algorithm>
#include <cstdio>

namespace nuage {

namespace {
constexpr float kPanelWidth = 580.0f;
constexpr float kPanelRadius = 18.0f;
constexpr float kPanelMargin = 28.0f;
constexpr float kRowHeight = 38.0f;
constexpr float kHeaderHeight = 52.0f;
constexpr int kRowCount = 16;
constexpr float kPanelPaddingBottom = 24.0f;
constexpr float kPaddingX = 20.0f;
constexpr float kHeaderTextY = 14.0f;
constexpr float kDividerY = 40.0f;
constexpr float kButtonSize = 24.0f;
constexpr float kButtonGap = 10.0f;
constexpr float kValueOffset = 220.0f;

const Vec3 kPanelColor = Vec3(0.08f, 0.1f, 0.14f);
const Vec3 kPanelOutline = Vec3(0.28f, 0.32f, 0.38f);
const Vec3 kDividerColor = Vec3(0.2f, 0.23f, 0.28f);
const Vec3 kButtonFill = Vec3(0.14f, 0.18f, 0.24f);
const Vec3 kButtonHover = Vec3(0.22f, 0.28f, 0.36f);
const Vec3 kTextMain = Vec3(0.95f, 0.96f, 0.98f);
const Vec3 kTextSub = Vec3(0.72f, 0.76f, 0.82f);

constexpr float kFogStep = 500.0f;
constexpr float kNoiseStep = 0.05f;
constexpr float kMacroScaleStep = 0.0001f;
constexpr float kMacroStrengthStep = 0.02f;
constexpr float kTintStrengthStep = 0.02f;
constexpr float kMicroScaleStep = 0.02f;
constexpr float kMicroStrengthStep = 0.02f;
constexpr float kWaterScaleStep = 0.02f;
constexpr float kWaterStrengthStep = 0.02f;
constexpr float kRollTrimStep = 0.02f;
constexpr const char* kDebugGroup = "debug";
} // namespace

void DebugOverlay::update(bool active, UIManager& ui) {
    if (!m_initialized) {
        buildUi(ui);
    }

    layout(ui);

    if (m_visible != active) {
        ui.setGroupActive(kDebugGroup, active);
        m_visible = active;
    }
}

void DebugOverlay::draw(bool active, UIManager& ui) {
    if (!active) return;

    ui.drawRoundedRect(m_panelX, m_panelY, kPanelWidth, m_panelHeight, kPanelRadius,
                       kPanelColor, 0.92f, Anchor::TopLeft);
    ui.drawRoundedRect(m_panelX, m_panelY, kPanelWidth, m_panelHeight, kPanelRadius,
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
        m_rowCount = 1;
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

    const auto& tex = terrain->textureSettings();

    char macroScaleBuffer[32];
    std::snprintf(macroScaleBuffer, sizeof(macroScaleBuffer), "%.4f", tex.macroScale);
    drawRow("Tex Macro Scale", macroScaleBuffer, row++);

    char macroStrengthBuffer[32];
    std::snprintf(macroStrengthBuffer, sizeof(macroStrengthBuffer), "%.2f", tex.macroStrength);
    drawRow("Tex Macro Strength", macroStrengthBuffer, row++);

    char grassTintBuffer[32];
    std::snprintf(grassTintBuffer, sizeof(grassTintBuffer), "%.2f", tex.grassTintStrength);
    drawRow("Grass Tint", grassTintBuffer, row++);

    char forestTintBuffer[32];
    std::snprintf(forestTintBuffer, sizeof(forestTintBuffer), "%.2f", tex.forestTintStrength);
    drawRow("Forest Tint", forestTintBuffer, row++);

    char urbanTintBuffer[32];
    std::snprintf(urbanTintBuffer, sizeof(urbanTintBuffer), "%.2f", tex.urbanTintStrength);
    drawRow("Urban Tint", urbanTintBuffer, row++);

    char microScaleBuffer[32];
    std::snprintf(microScaleBuffer, sizeof(microScaleBuffer), "%.2f", tex.microScale);
    drawRow("Tex Micro Scale", microScaleBuffer, row++);

    char microStrengthBuffer[32];
    std::snprintf(microStrengthBuffer, sizeof(microStrengthBuffer), "%.2f", tex.microStrength);
    drawRow("Tex Micro Strength", microStrengthBuffer, row++);

    char waterScaleBuffer[32];
    std::snprintf(waterScaleBuffer, sizeof(waterScaleBuffer), "%.2f", tex.waterDetailScale);
    drawRow("Water Detail Scale", waterScaleBuffer, row++);

    char waterStrengthBuffer[32];
    std::snprintf(waterStrengthBuffer, sizeof(waterStrengthBuffer), "%.2f", tex.waterDetailStrength);
    drawRow("Water Detail Strength", waterStrengthBuffer, row++);

    drawRow("Trees", terrain->treesEnabled() ? "On" : "Off", row++);
    double rollTrim = PropertyBus::global().get(Properties::Controls::ROLL_TRIM, 0.0);
    char rollTrimBuffer[32];
    std::snprintf(rollTrimBuffer, sizeof(rollTrimBuffer), "%.2f", rollTrim);
    drawRow("Roll Trim", rollTrimBuffer, row++);
    drawRow("Sound", PropertyBus::global().get(Properties::Audio::MUTED, false) ? "Off" : "On", row++);
    m_rowCount = row;

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
            .scaleVal(0.7f)
            .groupName(kDebugGroup);
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
    m_macroScaleMinus = makeButton("-");
    m_macroScalePlus = makeButton("+");
    m_macroStrengthMinus = makeButton("-");
    m_macroStrengthPlus = makeButton("+");
    m_grassTintMinus = makeButton("-");
    m_grassTintPlus = makeButton("+");
    m_forestTintMinus = makeButton("-");
    m_forestTintPlus = makeButton("+");
    m_urbanTintMinus = makeButton("-");
    m_urbanTintPlus = makeButton("+");
    m_microScaleMinus = makeButton("-");
    m_microScalePlus = makeButton("+");
    m_microStrengthMinus = makeButton("-");
    m_microStrengthPlus = makeButton("+");
    m_waterScaleMinus = makeButton("-");
    m_waterScalePlus = makeButton("+");
    m_waterStrengthMinus = makeButton("-");
    m_waterStrengthPlus = makeButton("+");
    m_treesMinus = makeButton("-");
    m_treesPlus = makeButton("+");
    m_audioMinus = makeButton("-");
    m_audioPlus = makeButton("+");
    m_rollTrimMinus = makeButton("-");
    m_rollTrimPlus = makeButton("+");

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
    if (m_macroScaleMinus) {
        m_macroScaleMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.macroScale = std::clamp(tex.macroScale - kMacroScaleStep, 0.0001f, 0.01f);
        });
    }
    if (m_macroScalePlus) {
        m_macroScalePlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.macroScale = std::clamp(tex.macroScale + kMacroScaleStep, 0.0001f, 0.01f);
        });
    }
    if (m_macroStrengthMinus) {
        m_macroStrengthMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.macroStrength = std::clamp(tex.macroStrength - kMacroStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_macroStrengthPlus) {
        m_macroStrengthPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.macroStrength = std::clamp(tex.macroStrength + kMacroStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_grassTintMinus) {
        m_grassTintMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.grassTintStrength = std::clamp(tex.grassTintStrength - kTintStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_grassTintPlus) {
        m_grassTintPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.grassTintStrength = std::clamp(tex.grassTintStrength + kTintStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_forestTintMinus) {
        m_forestTintMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.forestTintStrength = std::clamp(tex.forestTintStrength - kTintStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_forestTintPlus) {
        m_forestTintPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.forestTintStrength = std::clamp(tex.forestTintStrength + kTintStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_urbanTintMinus) {
        m_urbanTintMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.urbanTintStrength = std::clamp(tex.urbanTintStrength - kTintStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_urbanTintPlus) {
        m_urbanTintPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.urbanTintStrength = std::clamp(tex.urbanTintStrength + kTintStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_microScaleMinus) {
        m_microScaleMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.microScale = std::clamp(tex.microScale - kMicroScaleStep, 0.01f, 2.0f);
        });
    }
    if (m_microScalePlus) {
        m_microScalePlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.microScale = std::clamp(tex.microScale + kMicroScaleStep, 0.01f, 2.0f);
        });
    }
    if (m_microStrengthMinus) {
        m_microStrengthMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.microStrength = std::clamp(tex.microStrength - kMicroStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_microStrengthPlus) {
        m_microStrengthPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.microStrength = std::clamp(tex.microStrength + kMicroStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_waterScaleMinus) {
        m_waterScaleMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.waterDetailScale = std::clamp(tex.waterDetailScale - kWaterScaleStep, 0.01f, 1.0f);
        });
    }
    if (m_waterScalePlus) {
        m_waterScalePlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.waterDetailScale = std::clamp(tex.waterDetailScale + kWaterScaleStep, 0.01f, 1.0f);
        });
    }
    if (m_waterStrengthMinus) {
        m_waterStrengthMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.waterDetailStrength = std::clamp(tex.waterDetailStrength - kWaterStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_waterStrengthPlus) {
        m_waterStrengthPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            auto& tex = terrain->textureSettings();
            tex.waterDetailStrength = std::clamp(tex.waterDetailStrength + kWaterStrengthStep, 0.0f, 1.0f);
        });
    }
    if (m_treesMinus) {
        m_treesMinus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            terrain->setTreesEnabled(false);
        });
    }
    if (m_treesPlus) {
        m_treesPlus->onClick([getTerrain]() {
            TerrainRenderer* terrain = getTerrain();
            if (!terrain) return;
            terrain->setTreesEnabled(true);
        });
    }
    if (m_rollTrimMinus) {
        m_rollTrimMinus->onClick([]() {
            double trim = PropertyBus::global().get(Properties::Controls::ROLL_TRIM, 0.0);
            trim = std::clamp(trim - kRollTrimStep, -1.0, 1.0);
            PropertyBus::global().set(Properties::Controls::ROLL_TRIM, trim);
        });
    }
    if (m_rollTrimPlus) {
        m_rollTrimPlus->onClick([]() {
            double trim = PropertyBus::global().get(Properties::Controls::ROLL_TRIM, 0.0);
            trim = std::clamp(trim + kRollTrimStep, -1.0, 1.0);
            PropertyBus::global().set(Properties::Controls::ROLL_TRIM, trim);
        });
    }
    if (m_audioMinus) {
        m_audioMinus->onClick([]() {
            PropertyBus::global().set(Properties::Audio::MUTED, true);
        });
    }
    if (m_audioPlus) {
        m_audioPlus->onClick([]() {
            PropertyBus::global().set(Properties::Audio::MUTED, false);
        });
    }

    m_initialized = true;
    ui.setGroupActive(kDebugGroup, false);
}

void DebugOverlay::layout(UIManager& ui) {
    int width = ui.getWindowWidth();
    int height = ui.getWindowHeight();
    int rowCount = m_rowCount > 0 ? m_rowCount : kRowCount;
    if (width == m_lastWidth && height == m_lastHeight && rowCount == m_lastRowCount) return;

    m_lastWidth = width;
    m_lastHeight = height;
    m_lastRowCount = rowCount;

    m_panelX = static_cast<float>(width) - kPanelMargin - kPanelWidth;
    m_panelY = kPanelMargin;
    m_panelHeight = kHeaderHeight + kRowHeight * rowCount + kPanelPaddingBottom;

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
    positionRowButtons(m_macroScaleMinus, m_macroScalePlus, 4);
    positionRowButtons(m_macroStrengthMinus, m_macroStrengthPlus, 5);
    positionRowButtons(m_grassTintMinus, m_grassTintPlus, 6);
    positionRowButtons(m_forestTintMinus, m_forestTintPlus, 7);
    positionRowButtons(m_urbanTintMinus, m_urbanTintPlus, 8);
    positionRowButtons(m_microScaleMinus, m_microScalePlus, 9);
    positionRowButtons(m_microStrengthMinus, m_microStrengthPlus, 10);
    positionRowButtons(m_waterScaleMinus, m_waterScalePlus, 11);
    positionRowButtons(m_waterStrengthMinus, m_waterStrengthPlus, 12);
    positionRowButtons(m_treesMinus, m_treesPlus, 13);
    positionRowButtons(m_rollTrimMinus, m_rollTrimPlus, 14);
    positionRowButtons(m_audioMinus, m_audioPlus, 15);
}

void DebugOverlay::setButtonsVisible(bool visible) {
    if (m_radiusMinus) visible ? m_radiusMinus->show() : m_radiusMinus->hide();
    if (m_radiusPlus) visible ? m_radiusPlus->show() : m_radiusPlus->hide();
    if (m_loadsMinus) visible ? m_loadsMinus->show() : m_loadsMinus->hide();
    if (m_loadsPlus) visible ? m_loadsPlus->show() : m_loadsPlus->hide();
    if (m_fogMinus) visible ? m_fogMinus->show() : m_fogMinus->hide();
    if (m_fogPlus) visible ? m_fogPlus->show() : m_fogPlus->hide();
    if (m_noiseMinus) visible ? m_noiseMinus->show() : m_noiseMinus->hide();
    if (m_noisePlus) visible ? m_noisePlus->show() : m_noisePlus->hide();
    if (m_macroScaleMinus) visible ? m_macroScaleMinus->show() : m_macroScaleMinus->hide();
    if (m_macroScalePlus) visible ? m_macroScalePlus->show() : m_macroScalePlus->hide();
    if (m_macroStrengthMinus) visible ? m_macroStrengthMinus->show() : m_macroStrengthMinus->hide();
    if (m_macroStrengthPlus) visible ? m_macroStrengthPlus->show() : m_macroStrengthPlus->hide();
    if (m_grassTintMinus) visible ? m_grassTintMinus->show() : m_grassTintMinus->hide();
    if (m_grassTintPlus) visible ? m_grassTintPlus->show() : m_grassTintPlus->hide();
    if (m_forestTintMinus) visible ? m_forestTintMinus->show() : m_forestTintMinus->hide();
    if (m_forestTintPlus) visible ? m_forestTintPlus->show() : m_forestTintPlus->hide();
    if (m_urbanTintMinus) visible ? m_urbanTintMinus->show() : m_urbanTintMinus->hide();
    if (m_urbanTintPlus) visible ? m_urbanTintPlus->show() : m_urbanTintPlus->hide();
    if (m_microScaleMinus) visible ? m_microScaleMinus->show() : m_microScaleMinus->hide();
    if (m_microScalePlus) visible ? m_microScalePlus->show() : m_microScalePlus->hide();
    if (m_microStrengthMinus) visible ? m_microStrengthMinus->show() : m_microStrengthMinus->hide();
    if (m_microStrengthPlus) visible ? m_microStrengthPlus->show() : m_microStrengthPlus->hide();
    if (m_waterScaleMinus) visible ? m_waterScaleMinus->show() : m_waterScaleMinus->hide();
    if (m_waterScalePlus) visible ? m_waterScalePlus->show() : m_waterScalePlus->hide();
    if (m_waterStrengthMinus) visible ? m_waterStrengthMinus->show() : m_waterStrengthMinus->hide();
    if (m_waterStrengthPlus) visible ? m_waterStrengthPlus->show() : m_waterStrengthPlus->hide();
    if (m_treesMinus) visible ? m_treesMinus->show() : m_treesMinus->hide();
    if (m_treesPlus) visible ? m_treesPlus->show() : m_treesPlus->hide();
    if (m_audioMinus) visible ? m_audioMinus->show() : m_audioMinus->hide();
    if (m_audioPlus) visible ? m_audioPlus->show() : m_audioPlus->hide();
    if (m_rollTrimMinus) visible ? m_rollTrimMinus->show() : m_rollTrimMinus->hide();
    if (m_rollTrimPlus) visible ? m_rollTrimPlus->show() : m_rollTrimPlus->hide();
}

} // namespace nuage
