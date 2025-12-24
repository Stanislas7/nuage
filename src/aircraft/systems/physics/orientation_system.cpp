#include "orientation_system.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"
#include "math/quat.hpp"

namespace nuage {

OrientationSystem::OrientationSystem(const OrientationConfig& config)
    : m_config(config)
{
}

void OrientationSystem::init(PropertyBus* state) {
    m_state = state;
}

void OrientationSystem::update(float dt) {
    double pitch = m_state->get(Properties::Input::PITCH);
    double yaw = m_state->get(Properties::Input::YAW);
    double roll = m_state->get(Properties::Input::ROLL);

    Quat orientation = m_state->getQuat(Properties::Orientation::PREFIX);

    Vec3 fwd = orientation.rotate(Vec3(0, 0, 1));
    Vec3 rgt = orientation.rotate(Vec3(1, 0, 0));

    float pitchDelta = static_cast<float>(pitch * m_config.pitchRate * dt);
    float yawDelta = static_cast<float>(yaw * m_config.yawRate * dt);
    float rollDelta = static_cast<float>(roll * m_config.rollRate * dt);

    Quat pitchRot = Quat::fromAxisAngle(rgt, pitchDelta);
    Quat yawRot = Quat::fromAxisAngle(Vec3(0, 1, 0), yawDelta);
    Quat rollRot = Quat::fromAxisAngle(fwd, rollDelta);

    Quat newOrientation = (yawRot * pitchRot * rollRot * orientation).normalized();

    m_state->setQuat(Properties::Orientation::PREFIX, newOrientation);
}

}
