#pragma once

#include "ui/ui_manager.hpp"

namespace nuage {

class Button;

/**
 * @brief Logic and visual management for the pause screen.
 */
class PauseOverlay {
public:
    void update(bool paused, UIManager& ui);
    void draw(bool paused, UIManager& ui);
    void reset();

private:
    void buildUi(UIManager& ui);
    void layout(UIManager& ui);
    void setButtonsVisible(bool visible);

    bool m_initialized = false;
    bool m_visible = false;
    int m_lastWidth = 0;
    int m_lastHeight = 0;
    float m_panelCenterY = 0.0f;
    Button* m_dawnButton = nullptr;
    Button* m_noonButton = nullptr;
    Button* m_duskButton = nullptr;
    Button* m_midnightButton = nullptr;
    Button* m_resumeButton = nullptr;
};

} // namespace nuage
