#pragma once

#include "aircraft/aircraft_component.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"

namespace nuage {

struct DragConfig {
    float dragCoefficient = 0.04f;
    float frontalArea = 4.0f;
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
