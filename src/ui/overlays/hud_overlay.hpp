#pragma once

#include "ui/ui_manager.hpp"

namespace nuage {

class Aircraft;
class UIManager;

class HudOverlay {
public:
    void draw(UIManager& ui, Aircraft& aircraft);
};

}
