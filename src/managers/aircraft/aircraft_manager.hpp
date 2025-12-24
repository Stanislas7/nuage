#pragma once

#include "aircraft/aircraft.hpp"
#include <vector>
#include <memory>
#include <string>

namespace nuage {

class App;

class AircraftManager {
public:
    void init(App* app);
    void fixedUpdate(float dt);
    void render();
    void shutdown();

    Aircraft* spawnPlayer(const std::string& configPath);

    Aircraft* player() { return m_player; }
    const std::vector<std::unique_ptr<Aircraft>>& all() const { return m_aircraft; }

    void destroy(Aircraft* aircraft);
    void destroyAll();

private:
    App* m_app = nullptr;
    std::vector<std::unique_ptr<Aircraft>> m_aircraft;
    Aircraft* m_player = nullptr;
};

}
