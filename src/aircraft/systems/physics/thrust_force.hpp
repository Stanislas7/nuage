#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct ThrustConfig {
    float thrustScale = 1.0f;
    float propEfficiency = 0.8f;
    float minAirspeed = 10.0f;
};

class ThrustForce : public AircraftComponent {
public:
    explicit ThrustForce(const ThrustConfig& config = {});

    const char* name() const override { return "ThrustForce"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    ThrustConfig m_config;
};

}
