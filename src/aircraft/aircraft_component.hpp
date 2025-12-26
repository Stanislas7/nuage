#pragma once

namespace nuage {

class PropertyBus;
struct AircraftState;

class AircraftComponent {
public:
    virtual ~AircraftComponent() = default;
    virtual const char* name() const = 0;
    virtual void init(AircraftState& state, PropertyBus& bus) = 0;
    virtual void update(float dt) = 0;
};

}
