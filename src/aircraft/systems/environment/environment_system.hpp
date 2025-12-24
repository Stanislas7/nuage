#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

class Atmosphere;

class EnvironmentSystem : public AircraftComponent {
public:
    explicit EnvironmentSystem(Atmosphere& atmosphere);

    const char* name() const override { return "Environment"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    Atmosphere* m_atmosphere = nullptr;
};

}
