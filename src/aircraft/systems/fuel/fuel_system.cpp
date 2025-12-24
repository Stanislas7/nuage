#include "fuel_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include <algorithm>

namespace nuage {

FuelSystem::FuelSystem(const FuelConfig& config)
    : m_config(config)
{
}

void FuelSystem::init(PropertyBus* state) {
    m_state = state;
    
    m_state->set(Properties::Fuel::QUANTITY, m_config.initialFuel);
    m_state->set(Properties::Fuel::CAPACITY, m_config.capacity);
}

void FuelSystem::update(float dt) {
    double fuelFlow = m_state->get(Properties::Engine::FUEL_FLOW);
    double quantity = m_state->get(Properties::Fuel::QUANTITY);
    
    quantity -= fuelFlow * dt;
    quantity = std::max(0.0, quantity);
    
    m_state->set(Properties::Fuel::QUANTITY, quantity);
    
    if (quantity <= 0.0) {
        m_state->set(Properties::Engine::RUNNING, 0.0);
    }
}

}
