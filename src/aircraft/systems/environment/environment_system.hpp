#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

class Atmosphere;

class EnvironmentSystem : public AircraftComponent {
public:
    explicit EnvironmentSystem(Atmosphere& atmosphere);

    const char* name() const override { return "EnvironmentSystem"; }
    void init(AircraftState& state, PropertyBus& bus) override;
    void update(float dt) override;

private:
    AircraftState* m_acState = nullptr;
    PropertyBus* m_bus = nullptr;
    Atmosphere* m_atmosphere = nullptr;
};

}
