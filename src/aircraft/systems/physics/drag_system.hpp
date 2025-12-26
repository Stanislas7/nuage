#pragma once

#include "aircraft/aircraft_component.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"

namespace nuage {

struct DragConfig {
    float cd0 = 0.0f;
    float cdStall = 0.0f;
    float inducedDragFactor = 0.0f;
    float wingArea = 0.0f;
    float frontalArea = 0.0f;
    float stallAlphaRad = 0.0f;
    float postStallAlphaRad = 0.0f;
};

class DragSystem : public AircraftComponent, private AerodynamicForceBase {
public:
    explicit DragSystem(const DragConfig& config = {});

    const char* name() const override { return "DragSystem"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    DragConfig m_config;
};

}
