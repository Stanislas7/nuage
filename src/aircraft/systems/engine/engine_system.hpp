#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct EngineConfig {
    float maxThrust = 0.0f;      // Newtons
    float maxPowerKw = 0.0f;         // kW (optional for prop-driven model)
    float idleN1 = 0.0f;            // % at idle
    float maxN1 = 0.0f;            // % at full throttle  
    float spoolRate = 0.0f;          // How fast N1 responds
    float fuelFlowIdle = 0.0f;       // kg/s at idle
    float fuelFlowMax = 0.0f;        // kg/s at max thrust
};

class EngineSystem : public AircraftComponent {
public:
    explicit EngineSystem(const EngineConfig& config = {});

    const char* name() const override { return "Engine"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    EngineConfig m_config;
    
    double m_currentN1 = 20.0;
};

}
