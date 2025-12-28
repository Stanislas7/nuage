#include "graphics/renderers/skybox.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/shader.hpp"
#include "graphics/glad.h"
#include "environment/atmosphere.hpp"

namespace nuage {

bool Skybox::init(AssetStore& assets) {
    m_shader = assets.getShader("sky");
    if (!m_shader) return false;

    glGenVertexArrays(1, &m_vao);
    return true;
}

void Skybox::shutdown() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_shader = nullptr;
}

void Skybox::render(const Mat4& view, const Mat4& proj, const Atmosphere& atmosphere, float time) {
    if (!m_shader || !m_vao) return;

    Vec3 right(view.m[0], view.m[4], view.m[8]);
    Vec3 up(view.m[1], view.m[5], view.m[9]);
    Vec3 forward(-view.m[2], -view.m[6], -view.m[10]);

    right = right.normalized();
    up = up.normalized();
    forward = forward.normalized();

    float tanHalfFov = (proj.m[5] != 0.0f) ? (1.0f / proj.m[5]) : 1.0f;
    float aspect = (proj.m[0] != 0.0f) ? (proj.m[5] / proj.m[0]) : 1.0f;

    Vec3 sunDir = atmosphere.getSunDirection();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    m_shader->use();
    m_shader->setVec3("uCameraRight", right);
    m_shader->setVec3("uCameraUp", up);
    m_shader->setVec3("uCameraForward", forward);
    m_shader->setFloat("uAspect", aspect);
    m_shader->setFloat("uTanHalfFov", tanHalfFov);
    m_shader->setVec3("uSunDir", sunDir);
    m_shader->setFloat("uTime", time);
    
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

} // namespace nuage
