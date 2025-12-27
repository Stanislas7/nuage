#pragma once

#include "math/mat4.hpp"
#include "math/vec3.hpp"

namespace nuage {

class AssetStore;
class Shader;
class Atmosphere;

/**
 * @brief Handles rendering of the procedural skybox.
 */
class Skybox {
public:
    bool init(AssetStore& assets);
    void render(const Mat4& viewMatrix, const Mat4& projectionMatrix,
                const Atmosphere& atmosphere, float time);

private:
    Shader* m_shader = nullptr;
    unsigned int m_vao = 0;
};

} // namespace nuage
