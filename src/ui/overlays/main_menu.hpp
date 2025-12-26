#pragma once

#include "ui/ui_manager.hpp"
#include <functional>

namespace nuage {

class App;

/**
 * @brief The initial landing screen for the application.
 */
class MainMenu {
public:
    using StartCallback = std::function<void()>;

    void init(App* app, StartCallback onStart);
    void update(bool active, UIManager& ui);
    void draw(bool active, UIManager& ui);

private:
    App* m_app = nullptr;
    StartCallback m_onStart;
    bool m_initialized = false;
    
    // We store pointers to our buttons to toggle their visibility/activity
    Button* m_startButton = nullptr;
    Button* m_quitButton = nullptr;
};

} // namespace nuage
