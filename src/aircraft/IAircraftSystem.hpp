#pragma once

namespace nuage {

class PropertyBus;
class App;

class IAircraftSystem {
public:
    virtual ~IAircraftSystem() = default;
    virtual const char* name() const = 0;
    virtual void init(PropertyBus* state, App* app) = 0;
    virtual void update(float dt) = 0;
};

}
