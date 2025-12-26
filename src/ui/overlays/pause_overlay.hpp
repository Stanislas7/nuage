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
        // state logic, maybe someday
    }

    void draw(bool paused, UIManager& ui) {
        if (!paused) return;

        // Draw background tint
        ui.drawRect(0, 0, (float)ui.getWindowWidth(), (float)ui.getWindowHeight(), 
                    Vec3(0.0f, 0.0f, 0.0f), 0.45f);

        // Draw text
        ui.drawText("PAUSED", 0, -40, Anchor::Center, 1.5f, Vec3(1, 1, 1), 1.0f);
        ui.drawText("Press SPACE to resume", 0, 40, Anchor::Center, 0.6f, Vec3(1, 1, 1), 1.0f);
    }

    void reset() {
        // No state to reset currently
    }
};

} // namespace nuage