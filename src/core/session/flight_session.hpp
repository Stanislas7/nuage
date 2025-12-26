#pragma once

#include "aircraft/aircraft.hpp"
#include "environment/atmosphere.hpp"
#include "graphics/camera.hpp"
#include "scenery/scenery.hpp"
#include "core/session/flight_config.hpp"
#include "graphics/renderers/skybox.hpp"
#include "graphics/renderers/terrain_renderer.hpp"
#include "math/mat4.hpp"

namespace nuage {

class App;
class Text;

/**
 * @brief Represents an active flight simulation session.
 */
class FlightSession {
public:
    FlightSession(App* app, const FlightConfig& config);
    ~FlightSession() = default;

    bool init();
    void update(float dt);
    void render(float alpha);

    Aircraft& aircraft() { return m_aircraft; }
    Camera& camera() { return m_camera; }
    Atmosphere& atmosphere() { return m_atmosphere; }
    Scenery& scenery() { return m_scenery; }

private:
    void setupHUD();
    void updateHUD();

    App* m_app = nullptr;
    FlightConfig m_config;

    Aircraft m_aircraft;
    Atmosphere m_atmosphere;
    Camera m_camera;
    Scenery m_scenery;

    Skybox m_skybox;
    TerrainRenderer m_terrain;

    // HUD Text pointers (owned by UIManager)
    Text* m_altitudeText = nullptr;
    Text* m_airspeedText = nullptr;
    Text* m_headingText = nullptr;
    Text* m_positionText = nullptr;
    Text* m_powerText = nullptr;
};

} // namespace nuage