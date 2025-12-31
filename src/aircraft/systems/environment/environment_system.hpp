#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

class Atmosphere;

class EnvironmentSystem : public AircraftComponent {
public:
    explicit EnvironmentSystem(Atmosphere& atmosphere);

    const char* name() const override { return "EnvironmentSystem"; }
    void init(AircraftState& state, PropertyContext& properties) override;
    void update(float dt) override;

private:
    AircraftState* m_acState = nullptr;
    PropertyContext* m_properties = nullptr;
    Atmosphere* m_atmosphere = nullptr;
};

}
