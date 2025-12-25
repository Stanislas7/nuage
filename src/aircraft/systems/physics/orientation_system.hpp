#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

struct OrientationConfig {
    float pitchRate = 0.0f;
    float yawRate = 0.0f;
    float rollRate = 0.0f;
    float controlRefSpeed = 0.0f;
    float minControlScale = 0.0f;
    float maxControlScale = 0.0f;
    float torqueMultiplier = 0.0f;
    float dampingFactor = 0.0f;
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
