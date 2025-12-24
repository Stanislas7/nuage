#include "environment_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "environment/atmosphere_manager.hpp"

namespace nuage {

EnvironmentSystem::EnvironmentSystem(AtmosphereManager& atmosphere)
    : m_atmosphere(&atmosphere)
{
}

void EnvironmentSystem::init(PropertyBus* state) {
    m_state = state;
}

void EnvironmentSystem::update(float dt) {
    float altitude = static_cast<float>(m_state->get(Properties::Position::Y));
    float density = m_atmosphere->getAirDensity(altitude);
    m_state->set(Properties::Atmosphere::DENSITY, density);
}

}