#pragma once

#include "aircraft/aircraft_component.hpp"
#include "math/vec3.hpp"

namespace nuage {

struct PhysicsConfig {
    float minAltitude = 5.0f;
    float maxClimbRate = 20.0f;
    float groundFriction = 0.02f;
    Vec3 inertia = {1.0f, 1.0f, 1.0f}; // Generic default, overrides should come from config
};

class PhysicsIntegrator : public AircraftComponent {
public:
    explicit PhysicsIntegrator(const PhysicsConfig& config = {});

    const char* name() const override { return "PhysicsIntegrator"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    void integrate(float dt);

    PropertyBus* m_state = nullptr;
    PhysicsConfig m_config;
};

}
