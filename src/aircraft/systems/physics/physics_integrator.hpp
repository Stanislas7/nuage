#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct PhysicsConfig {
    float minAltitude = 5.0f;
    float maxClimbRate = 20.0f;
};

class PhysicsIntegrator : public AircraftComponent {
public:
    explicit PhysicsIntegrator(const PhysicsConfig& config = {});

    const char* name() const override { return "PhysicsIntegrator"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    void clearForces();
    void integrate(float dt);

    PropertyBus* m_state = nullptr;
    PhysicsConfig m_config;
    int m_phase = 0;
};

}
