#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct FlightDynamicsConfig {
    float minSpeed = 20.0f;
    float maxSpeed = 80.0f;
    float pitchRate = 1.5f;
    float yawRate = 1.0f;
    float rollRate = 2.0f;
    float maxPitch = 1.2f;
    float minAltitude = 5.0f;
    float throttleResponse = 0.5f;
    float gravity = 9.81f;
    float liftCoefficient = 0.004f;
    float wingArea = 16.0f;
    float cruiseSpeed = 50.0f;
};

class FlightDynamics : public AircraftComponent {
public:
    explicit FlightDynamics(const FlightDynamicsConfig& config = {});

    const char* name() const override { return "FlightDynamics"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    void updateThrottle(float dt);
    void updateOrientation(float dt);
    void updatePosition(float dt);
    void enforceConstraints();

    PropertyBus* m_state = nullptr;
    FlightDynamicsConfig m_config;
};

}
