#pragma once

#include "aircraft/aircraft_component.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"

namespace nuage {

struct LiftConfig {
    float cl0 = 0.2f;
    float clAlpha = 5.5f;
    float clMax = 1.5f;
    float clMin = -1.2f;
    float wingArea = 16.0f;
    bool useLegacyConstant = false;
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
