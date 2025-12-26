#include "environment_system.hpp"
#include "core/properties/property_bus.hpp"
#include "core/properties/property_paths.hpp"
#include "environment/atmosphere.hpp"
#include "aircraft/aircraft_state.hpp"

namespace nuage {

EnvironmentSystem::EnvironmentSystem(Atmosphere& atmosphere)
    : m_atmosphere(&atmosphere)
{
}

void EnvironmentSystem::init(AircraftState& state, PropertyBus& bus) {
    m_acState = &state;
    m_bus = &bus;
}

void EnvironmentSystem::update(float dt) {
    float altitude = m_acState->position.y;
    float density = m_atmosphere->getAirDensity(altitude);
    m_bus->set(Properties::Atmosphere::DENSITY, density);

    Vec3 wind = m_atmosphere->getWind(m_acState->position);
    m_bus->set(Properties::Atmosphere::WIND_PREFIX, wind);
}

}
