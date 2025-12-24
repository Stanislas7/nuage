#include "managers/aircraft/aircraft_manager.hpp"
#include "app/app.hpp"

namespace nuage {

void AircraftManager::init(App* app) {
    m_app = app;
}

void AircraftManager::fixedUpdate(float dt) {
    for (auto& ac : m_aircraft) {
        const FlightInput& input = m_app->input().flight();
        ac->update(dt, input);
    }
}

void AircraftManager::render() {
    for (auto& ac : m_aircraft) {
        ac->render(m_app->camera().viewProjection());
    }
}

void AircraftManager::shutdown() {
    destroyAll();
}

Aircraft* AircraftManager::spawnPlayer(const std::string& configPath) {
    auto aircraft = std::make_unique<Aircraft>();
    aircraft->init(configPath, m_app);
    
    m_player = aircraft.get();
    m_aircraft.push_back(std::move(aircraft));
    return m_player;
}

void AircraftManager::destroy(Aircraft* aircraft) {
    auto it = std::find_if(m_aircraft.begin(), m_aircraft.end(),
        [aircraft](const std::unique_ptr<Aircraft>& ac) { return ac.get() == aircraft; });
    
    if (it != m_aircraft.end()) {
        if (m_player == aircraft) {
            m_player = nullptr;
        }
        m_aircraft.erase(it);
    }
}

void AircraftManager::destroyAll() {
    m_aircraft.clear();
    m_player = nullptr;
}

}
