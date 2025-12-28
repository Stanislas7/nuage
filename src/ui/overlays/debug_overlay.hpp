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
    float m_panelX = 0.0f;
    float m_panelY = 0.0f;

    Button* m_radiusMinus = nullptr;
    Button* m_radiusPlus = nullptr;
    Button* m_loadsMinus = nullptr;
    Button* m_loadsPlus = nullptr;
    Button* m_fogMinus = nullptr;
    Button* m_fogPlus = nullptr;
    Button* m_noiseMinus = nullptr;
    Button* m_noisePlus = nullptr;
};

} // namespace nuage
