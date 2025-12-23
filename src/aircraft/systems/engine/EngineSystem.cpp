#include "EngineSystem.hpp"
#include "aircraft/property_bus.hpp"
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
    
    m_state->set("engine/n1", m_config.idleN1);
    m_state->set("engine/thrust", 0.0);
    m_state->set("engine/running", 1.0);
}

void EngineSystem::update(float dt) {
    bool running = m_state->get("engine/running") > 0.5;
    if (!running) {
        m_state->set("engine/n1", 0.0);
        m_state->set("engine/thrust", 0.0);
        m_state->set("engine/fuel_flow", 0.0);
        return;
    }

    double throttle = m_state->get("input/throttle");
    double targetN1 = m_config.idleN1 + (m_config.maxN1 - m_config.idleN1) * throttle;

    double n1Delta = (targetN1 - m_currentN1) * m_config.spoolRate;
    m_currentN1 = std::clamp(m_currentN1 + n1Delta * dt, 0.0, static_cast<double>(m_config.maxN1));
    
    double thrustRatio = (m_currentN1 - m_config.idleN1) / (m_config.maxN1 - m_config.idleN1);
    thrustRatio = std::max(0.0, thrustRatio);
    
    double density = m_state->get("atmosphere/density", 1.225);
    double densityRatio = density / 1.225;
    
    double thrust = m_config.maxThrust * thrustRatio * densityRatio;
    
    double n1Ratio = m_currentN1 / m_config.maxN1;
    double fuelFlow = m_config.fuelFlowIdle + 
        (m_config.fuelFlowMax - m_config.fuelFlowIdle) * n1Ratio * n1Ratio;

    m_state->set("engine/n1", m_currentN1);
    m_state->set("engine/thrust", thrust);
    m_state->set("engine/fuel_flow", fuelFlow);
}

}
