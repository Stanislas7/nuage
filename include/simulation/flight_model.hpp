#pragma once

#include "aircraft_state.hpp"
#include "flight_params.hpp"
#include "modules/input_module.hpp"  // For FlightControls

namespace flightsim {

// Pure flight dynamics - NO OpenGL, fully testable
class FlightModel {
public:
    void init(const FlightParams& params);
    void update(float dt, const FlightControls& controls);

    // State access
    const AircraftState& state() const { return m_state; }
    AircraftState& state() { return m_state; }

    // Convenience
    Vec3 position() const { return m_state.position; }
    Quat orientation() const { return m_state.orientation; }
    float speed() const { return m_state.speed; }
    float altitude() const { return m_state.position.y; }

    Vec3 forward() const;
    Vec3 up() const;
    Vec3 right() const;

    const FlightParams& params() const { return m_params; }

private:
    void updateThrottle(float dt, float throttleInput);
    void updateOrientation(float dt, const FlightControls& controls);
    void updatePosition(float dt);
    void enforceConstraints();

    FlightParams m_params;
    AircraftState m_state;
};

}
