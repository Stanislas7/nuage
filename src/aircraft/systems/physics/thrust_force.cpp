#include "thrust_force.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "math/quat.hpp"
#include <algorithm>

namespace nuage {

ThrustForce::ThrustForce(const ThrustConfig& config)
    : m_config(config)
{
}

void ThrustForce::init(PropertyBus* state) {
    m_state = state;
}

void ThrustForce::update(float dt) {
    double thrust = m_state->get(Properties::Engine::THRUST, 0.0);
    double power = m_state->get(Properties::Engine::POWER, 0.0);
    float thrustForce = 0.0f;
    if (power > 0.0) {
        Vec3 velocity = m_state->getVec3(Properties::Velocity::PREFIX);
        Vec3 wind = m_state->getVec3(Properties::Atmosphere::WIND_PREFIX);
        float airspeed = (velocity - wind).length();
        float effectiveSpeed = std::max(airspeed, m_config.minAirspeed);
        double density = m_state->get(Properties::Atmosphere::DENSITY, 1.225);
        double densityRatio = density / 1.225;
        thrustForce = static_cast<float>((power * m_config.propEfficiency * densityRatio) / effectiveSpeed);
        if (m_config.maxStaticThrust > 0.0f) {
            thrustForce = std::min(thrustForce, m_config.maxStaticThrust);
        }
    } else {
        thrustForce = static_cast<float>(thrust);
    }
    thrustForce *= m_config.thrustScale;

    Quat orientation = m_state->getQuat(Properties::Orientation::PREFIX);
    Vec3 forward = orientation.rotate(Vec3(0, 0, 1));
    Vec3 thrustVec = forward * thrustForce;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce = currentForce + thrustVec;
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);

    m_state->setVec3(Properties::Forces::THRUST_PREFIX, thrustVec);
}

}
