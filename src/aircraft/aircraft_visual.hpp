#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "math/mat4.hpp"
#include <string>

namespace nuage {

class AssetStore;
class Mesh;
class Shader;
class Texture;
class Model;

/**
 * @brief Handles the visual representation and rendering of an aircraft.
 * Separates rendering logic and OpenGL state management from physics/simulation.
 */
class AircraftVisual {
public:
    void init(const std::string& configPath, AssetStore& assets);
    void draw(const Vec3& position, const Quat& orientation, const Mat4& viewProjection);

private:
    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Texture* m_texture = nullptr;
    Model* m_model = nullptr;
    Shader* m_texturedShader = nullptr;
    
    Vec3 m_color = {1, 1, 1};
    Vec3 m_modelScale = {1, 1, 1};
    Quat m_modelRotation = Quat::identity();
    Vec3 m_modelOffset = {0, 0, 0};
};

} // namespace nuage
