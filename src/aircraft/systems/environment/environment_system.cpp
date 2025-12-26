#include "environment_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "environment/atmosphere.hpp"

namespace nuage {

EnvironmentSystem::EnvironmentSystem(Atmosphere& atmosphere)
    : m_atmosphere(&atmosphere)
{
}

void EnvironmentSystem::init(PropertyBus* state) {
    m_state = state;
}

void EnvironmentSystem::update(float dt) {
    Vec3 position = m_state->get(Properties::Position::PREFIX);
    float altitude = position.y;
    float density = m_atmosphere->getAirDensity(altitude);
    m_state->set(Properties::Atmosphere::DENSITY, density);

    Vec3 wind = m_atmosphere->getWind(position);
    m_state->set(Properties::Atmosphere::WIND_PREFIX, wind);
}

}