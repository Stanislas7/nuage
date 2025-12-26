#include "lift_system.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include <algorithm>
#include <cmath>

namespace nuage {

LiftSystem::LiftSystem(const LiftConfig& config)
    : m_config(config)
{
}

void LiftSystem::init(PropertyBus* state) {
    m_state = state;
}

void LiftSystem::update(float dt) {
    AerodynamicForceBase::AerodynamicData data = computeAerodynamics(m_state);

    if (data.airSpeed < 1.0f) {
        m_state->set(Properties::Aero::AOA, static_cast<double>(data.aoa));
        m_state->set(Properties::Aero::SIDESLIP, static_cast<double>(data.sideslip));
        m_state->set(Properties::Aero::CL, 0.0);
        return;
    }

    if (data.forwardSpeed < 0.1f) {
        m_state->set(Properties::Aero::AOA, static_cast<double>(data.aoa));
        m_state->set(Properties::Aero::SIDESLIP, static_cast<double>(data.sideslip));
        m_state->set(Properties::Aero::CL, 0.0);
        return;
    }

    float cl = m_config.cl0 + m_config.clAlpha * data.aoa;

    float stallAlphaPos = m_config.stallAlphaRad;
    if (stallAlphaPos <= 0.0f && std::abs(m_config.clAlpha) > 0.0001f) {
        stallAlphaPos = (m_config.clMax - m_config.cl0) / m_config.clAlpha;
    }
    if (stallAlphaPos <= 0.0f) {
        stallAlphaPos = 0.35f;
    }

    float postStallAlphaPos = m_config.postStallAlphaRad;
    if (postStallAlphaPos <= stallAlphaPos) {
        postStallAlphaPos = stallAlphaPos + 0.35f;
    }

    float stallAlphaNeg = -stallAlphaPos;
    if (std::abs(m_config.clAlpha) > 0.0001f) {
        float derivedNeg = (m_config.clMin - m_config.cl0) / m_config.clAlpha;
        if (derivedNeg < 0.0f) {
            stallAlphaNeg = derivedNeg;
        }
    }

    float postStallAlphaNeg = stallAlphaNeg - (postStallAlphaPos - stallAlphaPos);

    if (data.aoa > stallAlphaPos) {
        float t = (data.aoa - stallAlphaPos) / std::max(postStallAlphaPos - stallAlphaPos, 0.001f);
        t = std::clamp(t, 0.0f, 1.0f);
        cl = (1.0f - t) * m_config.clMax + t * m_config.clPostStall;
    } else if (data.aoa < stallAlphaNeg) {
        float t = (data.aoa - stallAlphaNeg) / std::min(postStallAlphaNeg - stallAlphaNeg, -0.001f);
        t = std::clamp(t, 0.0f, 1.0f);
        cl = (1.0f - t) * m_config.clMin + t * m_config.clPostStallNeg;
    } else {
        cl = std::clamp(cl, m_config.clMin, m_config.clMax);
    }

    float liftMagnitude = cl * data.dynamicPressure * m_config.wingArea;

    Vec3 liftDir = data.up - data.airflowDir * data.up.dot(data.airflowDir);
    if (liftDir.length() < 0.001f) {
        m_state->set(Properties::Aero::AOA, static_cast<double>(data.aoa));
        m_state->set(Properties::Aero::SIDESLIP, static_cast<double>(data.sideslip));
        m_state->set(Properties::Aero::CL, 0.0);
        return;
    }
    liftDir = liftDir.normalize();
    if (liftDir.dot(data.up) < 0.0f) {
        liftDir = -liftDir;
    }

    Vec3 liftVec = liftDir * liftMagnitude;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce = currentForce + liftVec;
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);

    m_state->setVec3(Properties::Forces::LIFT_PREFIX, liftVec);
    m_state->set(Properties::Aero::AOA, static_cast<double>(data.aoa));
    m_state->set(Properties::Aero::SIDESLIP, static_cast<double>(data.sideslip));
    m_state->set(Properties::Aero::CL, static_cast<double>(cl));
}

}
