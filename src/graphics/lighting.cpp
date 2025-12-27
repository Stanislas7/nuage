#include "graphics/lighting.hpp"
#include "graphics/shader.hpp"

#include <algorithm>

namespace nuage {

void applyDirectionalLighting(Shader* shader, const Vec3& sunDir) {
    if (!shader) return;

    Vec3 dir = sunDir.normalized();
    float elevation = std::clamp(dir.y * 0.5f + 0.5f, 0.0f, 1.0f);

    Vec3 lightColor = Vec3(0.6f, 0.7f, 0.85f) * (1.0f - elevation)
        + Vec3(1.0f, 0.95f, 0.85f) * elevation;
    Vec3 ambientColor = Vec3(0.12f, 0.16f, 0.22f) * (1.0f - elevation)
        + Vec3(0.33f, 0.34f, 0.36f) * elevation;

    shader->setVec3("uLightDir", dir);
    shader->setVec3("uLightColor", lightColor);
    shader->setVec3("uAmbientColor", ambientColor);
}

} // namespace nuage
