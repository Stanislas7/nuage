#include "EngineSystem.hpp"
#include "aircraft/property_bus.hpp"
#include "aircraft/PropertyPaths.hpp"
#include <algorithm>
#include <cmath>

namespace nuage {

EngineSystem::EngineSystem(const EngineConfig& config)
    : m_config(config)
{
}

void EngineSystem::init(PropertyBus* state, App* app) {
    m_state = state;
    m_app = app;
    
    m_state->set(Properties::Engine::N1, m_config.idleN1);
    m_state->set(Properties::Engine::THRUST, 0.0);
    m_state->set(Properties::Engine::RUNNING, 1.0);
}

void EngineSystem::update(float dt) {
    bool running = m_state->get(Properties::Engine::RUNNING) > 0.5;
    if (!running) {
        m_state->set(Properties::Engine::N1, 0.0);
        m_state->set(Properties::Engine::THRUST, 0.0);
        m_state->set(Properties::Engine::FUEL_FLOW, 0.0);
        return;
    }

    double throttle = m_state->get(Properties::Input::THROTTLE);
    double targetN1 = m_config.idleN1 + (m_config.maxN1 - m_config.idleN1) * throttle;

    double decay = std::exp(-m_config.spoolRate * dt);
    m_currentN1 = targetN1 + (m_currentN1 - targetN1) * decay;
    m_currentN1 = std::clamp(m_currentN1, 0.0, static_cast<double>(m_config.maxN1));
    
    double thrustRatio = (m_currentN1 - m_config.idleN1) / (m_config.maxN1 - m_config.idleN1);
    thrustRatio = std::max(0.0, thrustRatio);
    
    double density = m_state->get(Properties::Atmosphere::DENSITY, 1.225);
    double densityRatio = density / 1.225;
    
    double thrust = m_config.maxThrust * thrustRatio * densityRatio;
    
    double n1Ratio = m_currentN1 / m_config.maxN1;
    double fuelFlow = m_config.fuelFlowIdle + 
        (m_config.fuelFlowMax - m_config.fuelFlowIdle) * n1Ratio * n1Ratio;

    m_state->set(Properties::Engine::N1, m_currentN1);
    m_state->set(Properties::Engine::THRUST, thrust);
    m_state->set(Properties::Engine::FUEL_FLOW, fuelFlow);
}

}
