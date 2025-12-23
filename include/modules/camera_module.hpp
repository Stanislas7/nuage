#pragma once

#include "core/module.hpp"
#include "graphics/render_context.hpp"
#include "math/vec3.hpp"
#include "math/mat4.hpp"

namespace flightsim {

enum class CameraMode {
    Chase,      // Behind and above target
    Cockpit,    // First person
    Free        // Debug free-fly
};

class CameraModule : public Module {
public:
    const char* name() const override { return "Camera"; }

    void init(Simulator* sim) override;
    void update(float dt) override;

    // Access
    const RenderContext& renderContext() const { return m_context; }
    Vec3 position() const { return m_position; }

    // Camera settings
    void setMode(CameraMode mode) { m_mode = mode; }
    void setFov(float fov) { m_fov = fov; }
    void setAspect(float aspect) { m_aspect = aspect; }

    // Chase camera settings
    void setChaseDistance(float d) { m_chaseDistance = d; }
    void setChaseHeight(float h) { m_chaseHeight = h; }
    void setChaseSmoothing(float s) { m_chaseSmoothing = s; }

private:
    void updateChaseCamera(float dt);
    void updateFreeCamera(float dt);
    void buildMatrices();

    CameraMode m_mode = CameraMode::Chase;
    RenderContext m_context;

    Vec3 m_position{0, 100, -50};
    Vec3 m_target{0, 100, 0};

    // Projection
    float m_fov = 60.0f * 3.14159f / 180.0f;
    float m_aspect = 16.0f / 9.0f;
    float m_near = 0.1f;
    float m_far = 5000.0f;

    // Chase camera
    float m_chaseDistance = 25.0f;
    float m_chaseHeight = 10.0f;
    float m_chaseSmoothing = 5.0f;
};

}
