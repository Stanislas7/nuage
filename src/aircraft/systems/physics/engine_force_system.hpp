#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct EngineForceConfig {
    float thrustScale = 1.0f;
};

class EngineForceSystem : public AircraftComponent {
public:
    explicit EngineForceSystem(const EngineForceConfig& config = {});

    const char* name() const override { return "EngineForceSystem"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    EngineForceConfig m_config;
};

}
