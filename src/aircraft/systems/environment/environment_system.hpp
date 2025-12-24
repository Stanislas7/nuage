#pragma once

#include "aircraft/aircraft_component.hpp"

namespace nuage {

class AtmosphereManager;

class EnvironmentSystem : public AircraftComponent {
public:
    explicit EnvironmentSystem(AtmosphereManager& atmosphere);

    const char* name() const override { return "Environment"; }
    void init(PropertyBus* state) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    AtmosphereManager* m_atmosphere = nullptr;
};

}
