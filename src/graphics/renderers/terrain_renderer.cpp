#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/mesh.hpp"
#include "graphics/mesh_builder.hpp"
#include "utils/config_loader.hpp"
#include <filesystem>
#include <iostream>

namespace nuage {

void TerrainRenderer::init(AssetStore& assets) {
    m_shader = assets.getShader("basic");
    m_texturedShader = assets.getShader("textured");
}

void TerrainRenderer::setup(const std::string& configPath, AssetStore& assets) {
    if (configPath.empty()) {
        auto terrainData = MeshBuilder::terrain(20000.0f, 40);
        assets.loadMesh("session_terrain", terrainData);
        m_mesh = assets.getMesh("session_terrain");
        return;
    }

    auto terrainConfigOpt = loadJsonConfig(configPath);
    if (terrainConfigOpt && terrainConfigOpt->contains("heightmap")) {
        const auto& config = *terrainConfigOpt;
        
        std::filesystem::path cfg(configPath);
        auto resolve = [&](const std::string& p) {
            if (p.empty()) return p;
            std::filesystem::path path(p);
            if (path.is_absolute()) return p;
            return (cfg.parent_path() / path).string();
        };

        std::string heightmap = resolve(config.value("heightmap", ""));
        float sizeX = config.value("sizeX", 20000.0f);
        float sizeZ = config.value("sizeZ", 20000.0f);
        float heightMin = config.value("heightMin", 0.0f);
        float heightMax = config.value("heightMax", 1000.0f);
        int maxResolution = config.value("maxResolution", 512);
        bool flipY = config.value("flipY", true);
        std::string albedo = resolve(config.value("albedo", ""));

        if (!albedo.empty() && assets.loadTexture("session_albedo", albedo)) {
            m_texture = assets.getTexture("session_albedo");
        }

        bool useTexture = (m_texture != nullptr);
        auto terrainData = MeshBuilder::terrainFromHeightmap(heightmap, sizeX, sizeZ, 
                                                             heightMin, heightMax, 
                                                             maxResolution, useTexture, flipY);
        if (!terrainData.empty()) {
            if (useTexture) {
                assets.loadTexturedMesh("session_terrain", terrainData);
                m_textured = true;
            } else {
                assets.loadMesh("session_terrain", terrainData);
            }
        }
    }

    m_mesh = assets.getMesh("session_terrain");
}

void TerrainRenderer::render(const Mat4& vp) {
    if (!m_mesh) return;

    Shader* shader = (m_textured && m_texturedShader && m_texture) ? m_texturedShader : m_shader;
    if (!shader) return;

    shader->use();
    shader->setMat4("uMVP", vp);
    if (m_textured && m_texture) {
        m_texture->bind(0);
        shader->setInt("uTexture", 0);
    }
    m_mesh->draw();
}

} // namespace nuage
