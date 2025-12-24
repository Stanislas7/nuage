#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct EngineConfig {
    float maxThrust = 50000.0f;      // Newtons
    float idleN1 = 20.0f;            // % at idle
    float maxN1 = 100.0f;            // % at full throttle  
    float spoolRate = 0.3f;          // How fast N1 responds
    float fuelFlowIdle = 0.1f;       // kg/s at idle
    float fuelFlowMax = 1.5f;        // kg/s at max thrust
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
