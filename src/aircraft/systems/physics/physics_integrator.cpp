#include "physics_integrator.hpp"
#include "aircraft/physics/physics_constants.hpp"
#include "core/property_bus.hpp"
#include "core/property_paths.hpp"

namespace nuage {

PhysicsIntegrator::PhysicsIntegrator(const PhysicsConfig& config)
    : m_config(config)
{
}

void PhysicsIntegrator::init(PropertyBus* state) {
    m_state = state;
}

void PhysicsIntegrator::update(float dt) {
    if (m_phase == 0) {
        clearForces();
        m_phase = 1;
    } else {
        integrate(dt);
        m_phase = 0;
    }
}

void PhysicsIntegrator::clearForces() {
    m_state->setVec3(Properties::Physics::FORCE_PREFIX, 0.0f, 0.0f, 0.0f);
    m_state->setVec3(Properties::Forces::TOTAL_PREFIX, 0.0f, 0.0f, 0.0f);
    m_state->setVec3(Properties::Forces::GRAVITY_PREFIX, 0.0f, 0.0f, 0.0f);
    m_state->setVec3(Properties::Forces::LIFT_PREFIX, 0.0f, 0.0f, 0.0f);
    m_state->setVec3(Properties::Forces::DRAG_PREFIX, 0.0f, 0.0f, 0.0f);
    m_state->setVec3(Properties::Forces::THRUST_PREFIX, 0.0f, 0.0f, 0.0f);
}

void PhysicsIntegrator::integrate(float dt) {
    Vec3 force = m_state->getVec3(Properties::Physics::FORCE_PREFIX);
    Vec3 position = m_state->getVec3(Properties::Position::PREFIX);
    double mass = m_state->get(Properties::Physics::MASS, 1000.0);

    if (position.y <= m_config.minAltitude + 0.1f) {
        Vec3 velocity = m_state->getVec3(Properties::Velocity::PREFIX);
        Vec3 horizontalVelocity = Vec3(velocity.x, 0.0f, velocity.z);
        float horizontalSpeed = horizontalVelocity.length();

        if (horizontalSpeed > 0.01f) {
            float frictionMagnitude = m_config.groundFriction * static_cast<float>(mass) * PhysicsConstants::GRAVITY;
            Vec3 frictionDir = -horizontalVelocity.normalize();
            Vec3 frictionForce = frictionDir * frictionMagnitude;

            force = force + frictionForce;
        }
    }

    Vec3 acceleration = force * (1.0f / static_cast<float>(mass));
    m_state->setVec3(Properties::Physics::ACCEL_PREFIX, acceleration);

    Vec3 velocity = m_state->getVec3(Properties::Velocity::PREFIX);
    velocity = velocity + acceleration * dt;
    m_state->setVec3(Properties::Velocity::PREFIX, velocity);

    float airSpeed = velocity.length();
    m_state->set(Properties::Physics::AIR_SPEED, static_cast<double>(airSpeed));

    position = position + velocity * dt;

    if (position.y < m_config.minAltitude) {
        position.y = m_config.minAltitude;
        if (velocity.y < 0.0f) {
            velocity.y = 0.0f;
        }
    }

    m_state->setVec3(Properties::Position::PREFIX, position);
}

}
