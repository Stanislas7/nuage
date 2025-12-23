#pragma once

#include "aircraft/IAircraftSystem.hpp"

namespace nuage {

class EnvironmentSystem : public IAircraftSystem {
public:
    const char* name() const override { return "Environment"; }
    void init(PropertyBus* state, App* app) override;
    void update(float dt) override;

private:
    PropertyBus* m_state = nullptr;
    App* m_app = nullptr;
};

}
