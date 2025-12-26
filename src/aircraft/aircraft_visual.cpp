#include "aircraft/aircraft_visual.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/texture.hpp"
#include "graphics/model.hpp"
#include "utils/config_loader.hpp"
#include "aircraft/aircraft_config_keys.hpp"
#include <iostream>

namespace nuage {

namespace {
    constexpr float kDegToRad = 3.1415926535f / 180.0f;

    Vec3 parseVec3(const json& value, const Vec3& fallback) {
        if (!value.is_array() || value.size() != 3) return fallback;
        return Vec3(
            value[0].get<float>(),
            value[1].get<float>(),
            value[2].get<float>()
        );
    }

    Vec3 parseScale(const json& value, const Vec3& fallback) {
        if (value.is_number()) {
            float s = value.get<float>();
            return Vec3(s, s, s);
        }
        return parseVec3(value, fallback);
    }

    Quat parseRotation(const json& value, const Quat& fallback) {
        if (!value.is_array() || value.size() != 3) return fallback;
        float rx = value[0].get<float>() * kDegToRad;
        float ry = value[1].get<float>() * kDegToRad;
        float rz = value[2].get<float>() * kDegToRad;
        Quat qx = Quat::fromAxisAngle(Vec3(1, 0, 0), rx);
        Quat qy = Quat::fromAxisAngle(Vec3(0, 1, 0), ry);
        Quat qz = Quat::fromAxisAngle(Vec3(0, 0, 1), rz);
        return (qz * qy * qx).normalized();
    }
}

void AircraftVisual::init(const std::string& configPath, AssetStore& assets) {
    auto jsonOpt = loadJsonConfig(configPath);
    if (!jsonOpt) return;

    const auto& json = *jsonOpt;
    if (!json.contains(ConfigKeys::MODEL)) return;

    const auto& mod = json[ConfigKeys::MODEL];
    std::string modelName = mod[ConfigKeys::NAME];
    std::string modelPath = mod[ConfigKeys::PATH];
    std::string modelTexturePath;
    bool modelHasTexcoords = false;
    
    if (assets.loadModel(modelName, modelPath, &modelTexturePath, &modelHasTexcoords)) {
        m_model = assets.getModel(modelName);
        if (!m_model || m_model->parts().empty()) {
            m_mesh = assets.getMesh(modelName);
        }
    }

    auto c = mod[ConfigKeys::COLOR];
    m_color = Vec3(c[0], c[1], c[2]);

    std::string texturePath = mod.contains(ConfigKeys::TEXTURE) ? mod[ConfigKeys::TEXTURE].get<std::string>() : modelTexturePath;
    if ((!m_model || m_model->parts().empty()) && !texturePath.empty() && modelHasTexcoords) {
        std::string textureName = modelName + "_diffuse";
        if (assets.loadTexture(textureName, texturePath)) {
            m_texture = assets.getTexture(textureName);
        }
    }
    
    m_modelScale = parseScale(mod[ConfigKeys::SCALE], m_modelScale);
    m_modelRotation = parseRotation(mod[ConfigKeys::ROTATION], m_modelRotation);
    m_modelOffset = parseVec3(mod[ConfigKeys::OFFSET], m_modelOffset);
    
    if (!m_mesh && (!m_model || m_model->parts().empty())) {
        m_mesh = assets.getMesh("aircraft");
    }
    m_shader = assets.getShader("basic");
    m_texturedShader = assets.getShader("textured");
}

void AircraftVisual::draw(const Vec3& position, const Quat& orientation, const Mat4& viewProjection) {
    Mat4 modelMatrix = Mat4::translate(position)
        * orientation.toMat4()
        * Mat4::translate(m_modelOffset)
        * m_modelRotation.toMat4()
        * Mat4::scale(m_modelScale.x, m_modelScale.y, m_modelScale.z);

    if (m_model && !m_model->parts().empty()) {
        for (const auto& part : m_model->parts()) {
            if (!part.mesh) continue;
            Shader* shader = (part.textured && part.texture && m_texturedShader) ? m_texturedShader : m_shader;
            if (!shader) continue;
            shader->use();
            shader->setMat4("uMVP", viewProjection * modelMatrix);
            if (part.textured && part.texture && shader == m_texturedShader) {
                part.texture->bind(0);
                shader->setInt("uTexture", 0);
            } else {
                shader->setVec3("uColor", m_color);
                shader->setBool("uUseUniformColor", true);
            }
            part.mesh->draw();
            if (shader == m_shader) {
                shader->setBool("uUseUniformColor", false);
            }
        }
        return;
    }

    if (!m_mesh || !m_shader) return;

    m_shader->use();
    m_shader->setMat4("uMVP", viewProjection * modelMatrix);
    if (m_texture && m_texturedShader) {
        m_texturedShader->use();
        m_texturedShader->setMat4("uMVP", viewProjection * modelMatrix);
        m_texture->bind(0);
        m_texturedShader->setInt("uTexture", 0);
        m_mesh->draw();
        return;
    }
    m_shader->setVec3("uColor", m_color);
    m_shader->setBool("uUseUniformColor", true);
    m_mesh->draw();
    m_shader->setBool("uUseUniformColor", false);
}

} // namespace nuage
