#pragma once

#include "math/vec3.hpp"
#include "math/mat4.hpp"
#include "aircraft/aircraft.hpp"

namespace nuage {

class Input;

enum class CameraMode {
    Chase,
    Cockpit,
    Tower,
    Orbit
};

class Camera {
public:
    void init(Input& input);
    void update(float dt, Aircraft::Instance* target = nullptr);

    void setMode(CameraMode mode) { m_mode = mode; }
    void setTarget(Aircraft::Instance* target) { m_target = target; }

    Mat4 viewMatrix() const { return m_view; }
    Mat4 projectionMatrix() const { return m_projection; }
    Mat4 viewProjection() const { return m_projection * m_view; }
    Vec3 position() const { return m_position; }

    void setFov(float fov) { m_fov = fov; }
    void setAspect(float aspect) { m_aspect = aspect; }
    void setChaseDistance(float d) { m_followDistance = d; }
    void setChaseHeight(float h) { m_followHeight = h; }

    void setOrbitDistance(float distance) { m_orbitDistance = distance; }
    void setOrbitSpeed(float speed) { m_orbitSpeed = speed; }
    void toggleOrbitMode();

    bool isOrbitMode() const { return m_mode == CameraMode::Orbit; }

private:
    void updateChaseCamera(float dt, Aircraft::Instance* target);
    void updateOrbitCamera(float dt, Aircraft::Instance* target);
    void buildMatrices();

    Input* m_input = nullptr;
    Aircraft::Instance* m_target = nullptr;
    CameraMode m_mode = CameraMode::Chase;

    Vec3 m_position{0, 100, -50};
    Vec3 m_lookAt{0, 100, 0};
    Mat4 m_view;
    Mat4 m_projection;

    float m_followDistance = 25.0f;
    float m_followHeight = 10.0f;
    float m_smoothing = 5.0f;

    float m_orbitDistance = 50.0f;
    float m_orbitSpeed = 2.0f;
    float m_orbitYaw = 0.0f;
    float m_orbitPitch = 0.0f;

    float m_fov = 60.0f * 3.14159f / 180.0f;
    float m_aspect = 16.0f / 9.0f;
    float m_near = 0.1f;
    float m_far = 5000.0f;
};

}
