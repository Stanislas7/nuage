#include "environment_system.hpp"
#include "aircraft/property_bus.hpp"
#include "aircraft/property_paths.hpp"
#include "managers/atmosphere/atmosphere_manager.hpp"

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