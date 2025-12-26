#include "ui/overlays/main_menu.hpp"
#include "core/app.hpp"

namespace nuage {

void MainMenu::init(App* app, StartCallback onStart) {
    m_app = app;
    m_onStart = std::move(onStart);
    m_initialized = true;

    m_startButton = &app->ui().button("START FLIGHT");
    m_startButton->pos(0, 40).anchorMode(Anchor::Center).size(400, 80).scaleVal(0.6f);
    m_startButton->colorR(0.2f, 0.6f, 0.3f).hoverColor(Vec3(0.3f, 0.8f, 0.4f));
    m_startButton->onClick([this]() {
        if (m_onStart) m_onStart();
    });

    m_quitButton = &app->ui().button("QUIT");
    m_quitButton->pos(0, 140).anchorMode(Anchor::Center).size(400, 80).scaleVal(0.6f);
    m_quitButton->colorR(0.6f, 0.2f, 0.2f).hoverColor(Vec3(0.8f, 0.3f, 0.3f));
    m_quitButton->onClick([this]() {
        m_app->quit();
    });
}

void MainMenu::update(bool active, UIManager& ui) {
    if (!m_initialized) return;

    m_startButton->setVisible(active).setEnabled(active);
    m_quitButton->setVisible(active).setEnabled(active);
}

void MainMenu::draw(bool active, UIManager& ui) {
    if (!active || !m_initialized) return;

    ui.drawRect(0, 0, (float)ui.getWindowWidth(), (float)ui.getWindowHeight(), 
                Vec3(0.05f, 0.07f, 0.1f), 0.9f);

    ui.drawText("Nuage", 0, -150, Anchor::Center, 2.5f, Vec3(1, 1, 1), 1.0f);
    ui.drawText("Alpha", 0, 240, Anchor::Center, 0.4f, Vec3(0.7f, 0.7f, 0.7f), 1.0f);
}

} // namespace nuage