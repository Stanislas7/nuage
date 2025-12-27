#pragma once

#include "math/mat4.hpp"
#include "math/vec3.hpp"
#include <string>

namespace nuage {

class AssetStore;
class Mesh;
class Shader;
class Texture;

/**
 * @brief Handles terrain mesh management and rendering for a session.
 */
class TerrainRenderer {
public:
    void init(AssetStore& assets);
    void setup(const std::string& configPath, AssetStore& assets);
    void render(const Mat4& viewProjection, const Vec3& sunDir);

private:
    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Shader* m_texturedShader = nullptr;
    Texture* m_texture = nullptr;
    bool m_textured = false;
};

} // namespace nuage
