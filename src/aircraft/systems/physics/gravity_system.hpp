#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

class GravitySystem : public AircraftComponent {
public:
    GravitySystem() = default;

    const char* name() const override { return "GravitySystem"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
};

}
