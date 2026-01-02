#pragma once

#include "ui/ui_manager.hpp"

namespace nuage {

class Button;
class UIManager;

class DebugOverlay {
public:
    void update(bool active, UIManager& ui);
    void draw(bool active, UIManager& ui);
    void reset();

private:
    void buildUi(UIManager& ui);
    void layout(UIManager& ui);
    void setButtonsVisible(bool visible);

    bool m_initialized = false;
    bool m_visible = false;
    int m_lastWidth = 0;
    int m_lastHeight = 0;
    int m_lastRowCount = 0;
    float m_panelX = 0.0f;
    float m_panelY = 0.0f;
    float m_panelHeight = 0.0f;
    int m_rowCount = 0;

    Button* m_radiusMinus = nullptr;
    Button* m_radiusPlus = nullptr;
    Button* m_loadsMinus = nullptr;
    Button* m_loadsPlus = nullptr;
    Button* m_fogMinus = nullptr;
    Button* m_fogPlus = nullptr;
    Button* m_noiseMinus = nullptr;
    Button* m_noisePlus = nullptr;
    Button* m_macroScaleMinus = nullptr;
    Button* m_macroScalePlus = nullptr;
    Button* m_macroStrengthMinus = nullptr;
    Button* m_macroStrengthPlus = nullptr;
    Button* m_grassTintMinus = nullptr;
    Button* m_grassTintPlus = nullptr;
    Button* m_forestTintMinus = nullptr;
    Button* m_forestTintPlus = nullptr;
    Button* m_urbanTintMinus = nullptr;
    Button* m_urbanTintPlus = nullptr;
    Button* m_microScaleMinus = nullptr;
    Button* m_microScalePlus = nullptr;
    Button* m_microStrengthMinus = nullptr;
    Button* m_microStrengthPlus = nullptr;
    Button* m_waterScaleMinus = nullptr;
    Button* m_waterScalePlus = nullptr;
    Button* m_waterStrengthMinus = nullptr;
    Button* m_waterStrengthPlus = nullptr;
    Button* m_treesMinus = nullptr;
    Button* m_treesPlus = nullptr;
    Button* m_audioMinus = nullptr;
    Button* m_audioPlus = nullptr;
    Button* m_rollTrimMinus = nullptr;
    Button* m_rollTrimPlus = nullptr;
    Button* m_maskViewMinus = nullptr;
    Button* m_maskViewPlus = nullptr;
};

} // namespace nuage
