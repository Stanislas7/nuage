#include "drag_system.hpp"
#include "aircraft/physics/forces/aerodynamic_force_base.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"

namespace nuage {

DragSystem::DragSystem(const DragConfig& config)
    : m_config(config)
{
}

void DragSystem::init(PropertyBus* state) {
    m_state = state;
}

void DragSystem::update(float dt) {
    AerodynamicForceBase::AerodynamicData data = computeAerodynamics(m_state);

    if (data.airSpeed < 0.01f) {
        return;
    }

    Vec3 dragDir = -data.airflowDir;

    float cl = static_cast<float>(m_state->get(Properties::Aero::CL, 0.0));
    float cd = m_config.cd0 + m_config.inducedDragFactor * cl * cl;
    float dragMagnitude = cd * data.dynamicPressure * m_config.wingArea;

    Vec3 dragVec = dragDir * dragMagnitude;

    Vec3 currentForce = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    currentForce = currentForce + dragVec;
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, currentForce);

    m_state->setVec3(Properties::Forces::DRAG_PREFIX, dragVec);
}

}
