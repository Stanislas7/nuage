#pragma once

#include "aircraft/aircraft_component.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"

namespace nuage {

struct LiftConfig {
    float cl0 = 0.0f;
    float clAlpha = 0.0f;
    float clMax = 0.0f;
    float clMin = 0.0f;
    float wingArea = 0.0f;
};

class LiftSystem : public AircraftComponent, private AerodynamicForceBase {
public:
    explicit LiftSystem(const LiftConfig& config = {});

    const char* name() const override { return "LiftSystem"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    LiftConfig m_config;
};

}
