#pragma once

#include "aircraft/IAircraftSystem.hpp"

namespace nuage {

struct FuelConfig {
    float capacity = 500.0f;         // kg
    float initialFuel = 400.0f;      // kg
};

class FuelSystem : public IAircraftSystem {
public:
    explicit FuelSystem(const FuelConfig& config = {});

    const char* name() const override { return "Fuel"; }
    void init(PropertyBus* state, App* app) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    App* m_app = nullptr;
    FuelConfig m_config;
};

}
