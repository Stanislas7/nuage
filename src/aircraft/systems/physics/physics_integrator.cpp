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
    if (!m_state->has(Properties::Physics::INERTIA)) {
        m_state->setVec3(Properties::Physics::INERTIA, m_config.inertia);
    }
    // Initialize angular velocity if not present
    if (!m_state->has(Properties::Physics::ANGULAR_VELOCITY_PREFIX)) {
        m_state->setVec3(Properties::Physics::ANGULAR_VELOCITY_PREFIX, 0.0, 0.0, 0.0);
    }
}

void PhysicsIntegrator::update(float dt) {
    integrate(dt);
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

    Vec3 torque = m_state->getVec3(Properties::Physics::TORQUE_PREFIX);
    Vec3 inertia = m_state->getVec3(Properties::Physics::INERTIA);
    Vec3 angularVel = m_state->getVec3(Properties::Physics::ANGULAR_VELOCITY_PREFIX);

    // Angular Acceleration = Torque / Inertia (component-wise approximation for principal axes)
    Vec3 angularAccel(
        torque.x / inertia.x,
        torque.y / inertia.y,
        torque.z / inertia.z
    );
    m_state->setVec3(Properties::Physics::ANGULAR_ACCEL_PREFIX, angularAccel);

    angularVel = angularVel + angularAccel * dt;
        
    m_state->setVec3(Properties::Physics::ANGULAR_VELOCITY_PREFIX, angularVel);

    // Integrate Orientation: q_new = q + 0.5 * q * w_body * dt
    Quat currentQ = m_state->getQuat(Properties::Orientation::PREFIX);
    Quat w(0.0f, angularVel.x, angularVel.y, angularVel.z);
    
    // q * w_body
    Quat dq = currentQ * w; 
    
    currentQ.w += dq.w * 0.5f * dt;
    currentQ.x += dq.x * 0.5f * dt;
    currentQ.y += dq.y * 0.5f * dt;
    currentQ.z += dq.z * 0.5f * dt;

    m_state->setQuat(Properties::Orientation::PREFIX, currentQ.normalized());
}

}