#include "FuelSystem.hpp"
#include "aircraft/property_bus.hpp"
#include <algorithm>

namespace nuage {

FuelSystem::FuelSystem(const FuelConfig& config)
    : m_config(config)
{
}

void FuelSystem::init(PropertyBus* state, App* app) {
    m_state = state;
    m_app = app;
    
    m_state->set("fuel/quantity", m_config.initialFuel);
    m_state->set("fuel/capacity", m_config.capacity);
}

void FuelSystem::update(float dt) {
    double fuelFlow = m_state->get("engine/fuel_flow");
    double quantity = m_state->get("fuel/quantity");
    
    quantity -= fuelFlow * dt;
    quantity = std::max(0.0, quantity);
    
    m_state->set("fuel/quantity", quantity);
    
    if (quantity <= 0.0) {
        m_state->set("engine/running", 0.0);
    }
}

}
