#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct GravityConfig {
    float gravity = 9.81f;
};

class GravitySystem : public AircraftComponent {
public:
    explicit GravitySystem(const GravityConfig& config = {});

    const char* name() const override { return "GravitySystem"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    GravityConfig m_config;
};

}
