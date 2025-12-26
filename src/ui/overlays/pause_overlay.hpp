#pragma once

#include "ui/ui_manager.hpp"
#include "math/vec3.hpp"
#include <string>

namespace nuage {

/**
 * @brief Logic and visual management for the pause screen.
 */
class PauseOverlay {
public:
    void update(bool paused, UIManager& ui) {
        if (paused == m_lastPaused) return;
        m_lastPaused = paused;

        std::string title = paused ? "PAUSED" : "";
        std::string hint = paused ? "Press SPACE to resume" : "";
        
        ui.setOverlayText(title, hint);
        ui.setOverlay(paused, Vec3(0.0f, 0.0f, 0.0f), paused ? 0.45f : 0.0f);
    }

private:
    bool m_lastPaused = false;
};

} // namespace nuage
