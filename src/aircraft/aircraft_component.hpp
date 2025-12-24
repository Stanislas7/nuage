#pragma once

namespace nuage {

class PropertyBus;
class App;

class AircraftComponent {
public:
    virtual ~AircraftComponent() = default;
    virtual const char* name() const = 0;
    virtual void init(PropertyBus* state) = 0;
    virtual void update(float dt) = 0;
};

}
