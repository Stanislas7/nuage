#pragma once

#include "aircraft/aircraft_component.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"

namespace nuage {

struct LiftConfig {
    float liftCoefficient = 0.4f;
    float wingArea = 16.0f;
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
