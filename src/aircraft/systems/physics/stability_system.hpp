#pragma once

#include "aircraft/aircraft_component.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"

namespace nuage {

struct StabilityConfig {
    float pitchStability = 0.0f;
    float yawStability = 0.0f;
    float rollStability = 0.0f;
    float pitchDamping = 0.0f;
    float yawDamping = 0.0f;
    float rollDamping = 0.0f;
    float referenceArea = 1.0f;
    float referenceLength = 1.0f;
    float momentScale = 1.0f;
    float minAirspeed = 5.0f;
};

class StabilitySystem : public AircraftComponent, private AerodynamicForceBase {
public:
    explicit StabilitySystem(const StabilityConfig& config = {});

    const char* name() const override { return "StabilitySystem"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    StabilityConfig m_config;
};

} // namespace nuage
