#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct OrientationConfig {
    float pitchRate = 1.5f;
    float yawRate = 1.0f;
    float rollRate = 2.0f;
    float controlRefSpeed = 55.0f;
    float minControlScale = 0.25f;
    float maxControlScale = 1.25f;
    float torqueMultiplier = 2000.0f;
    float dampingFactor = 2000.0f;
};

class OrientationSystem : public AircraftComponent {
public:
    explicit OrientationSystem(const OrientationConfig& config = {});

    const char* name() const override { return "OrientationSystem"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    OrientationConfig m_config;
};

}
