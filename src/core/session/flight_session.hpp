#pragma once

#include "aircraft/aircraft.hpp"
#include "environment/atmosphere.hpp"
#include "graphics/camera.hpp"
#include "core/session/flight_config.hpp"
#include "graphics/renderers/skybox.hpp"
#include "graphics/renderers/terrain_renderer.hpp"
#include "math/mat4.hpp"

namespace nuage {

class App;
class Text;
class UIManager;

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
    void drawHUD(UIManager& ui);

    Aircraft& aircraft() { return m_aircraft; }
    Camera& camera() { return m_camera; }
    Atmosphere& atmosphere() { return m_atmosphere; }
private:
    void setupHUD();
    void updateHUD();

    App* m_app = nullptr;
    FlightConfig m_config;

    Aircraft m_aircraft;
    Atmosphere m_atmosphere;
    Camera m_camera;

    Skybox m_skybox;
    TerrainRenderer m_terrain;

    // HUD Text pointers (owned by UIManager)
    Text* m_altitudeText = nullptr;
    Text* m_airspeedText = nullptr;
    Text* m_headingText = nullptr;
    Text* m_positionText = nullptr;
    float m_powerPercent = -1.0f;
    float m_elapsedTime = 0.0f;
};

} // namespace nuage
