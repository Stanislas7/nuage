#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct FuelConfig {
    float capacity = 500.0f;         // kg
    float initialFuel = 400.0f;      // kg
};

class FuelSystem : public AircraftComponent {
public:
    explicit FuelSystem(const FuelConfig& config = {});

    const char* name() const override { return "Fuel"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    FuelConfig m_config;
};

}
