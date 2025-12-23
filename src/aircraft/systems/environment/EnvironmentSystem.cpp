#include "EnvironmentSystem.hpp"
#include "aircraft/property_bus.hpp"
#include "aircraft/PropertyPaths.hpp"
#include "app/App.hpp"

namespace nuage {

void EnvironmentSystem::init(PropertyBus* state, App* app) {
    m_state = state;
    m_app = app;
}

void EnvironmentSystem::update(float dt) {
    float altitude = static_cast<float>(m_state->get(Properties::Position::Y));
    float density = m_app->atmosphere().getAirDensity(altitude);
    m_state->set(Properties::Atmosphere::DENSITY, density);
}

}
