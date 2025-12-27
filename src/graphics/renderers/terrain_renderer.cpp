#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/mesh.hpp"
#include "graphics/mesh_builder.hpp"
#include "graphics/lighting.hpp"
#include "utils/config_loader.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace nuage {

void TerrainRenderer::init(AssetStore& assets) {
    m_shader = assets.getShader("basic");
    m_texturedShader = assets.getShader("textured");
    m_assets = &assets;
}

void TerrainRenderer::setup(const std::string& configPath, AssetStore& assets) {
    m_assets = &assets;
    m_tiled = false;
    m_tileCache.clear();
    m_heightLayer = {};
    m_albedoLayer = {};
    m_mesh = nullptr;
    m_texture = nullptr;
    m_textured = false;

    if (configPath.empty()) {
        auto terrainData = MeshBuilder::terrain(20000.0f, 40);
        assets.loadMesh("session_terrain", terrainData);
        m_mesh = assets.getMesh("session_terrain");
        return;
    }

    auto terrainConfigOpt = loadJsonConfig(configPath);
    if (terrainConfigOpt && terrainConfigOpt->contains("tiledManifest")) {
        setupTiled(configPath, assets);
        if (m_tiled) {
            return;
        }
    }
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

void TerrainRenderer::render(const Mat4& vp, const Vec3& sunDir, const Vec3& cameraPos) {
    if (m_tiled) {
        renderTiled(vp, sunDir, cameraPos);
        return;
    }
    if (!m_mesh) return;

    Shader* shader = (m_textured && m_texturedShader && m_texture) ? m_texturedShader : m_shader;
    if (!shader) return;

    shader->use();
    shader->setMat4("uMVP", vp);
    applyDirectionalLighting(shader, sunDir);
    if (m_textured && m_texture) {
        m_texture->bind(0);
        shader->setInt("uTexture", 0);
    }
    m_mesh->draw();
}

void TerrainRenderer::setupTiled(const std::string& configPath, AssetStore& assets) {
    auto terrainConfigOpt = loadJsonConfig(configPath);
    if (!terrainConfigOpt) {
        return;
    }
    const auto& config = *terrainConfigOpt;

    std::filesystem::path cfg(configPath);
    auto resolve = [&](const std::string& p) {
        if (p.empty()) return p;
        std::filesystem::path path(p);
        if (path.is_absolute()) return p;
        return (cfg.parent_path() / path).string();
    };

    std::string manifestPath = resolve(config.value("tiledManifest", ""));
    if (manifestPath.empty()) {
        return;
    }
    auto manifestOpt = loadJsonConfig(manifestPath);
    if (!manifestOpt) {
        std::cerr << "Failed to load terrain manifest: " << manifestPath << "\n";
        return;
    }

    const auto& manifest = *manifestOpt;
    if (!manifest.contains("layers")) {
        std::cerr << "Terrain manifest missing layers: " << manifestPath << "\n";
        return;
    }

    m_manifestDir = std::filesystem::path(manifestPath).parent_path().string();
    m_tileSizePx = manifest.value("tileSize", 512);
    m_tileMaxResolution = config.value("tileMaxResolution", 128);
    m_tileFlipY = config.value("tileFlipY", true);
    m_tileViewDistance = config.value("tileViewDistance", 8000.0f);
    m_tileLodDistance = config.value("tileLodDistance", 0.0f);
    m_tileMinLod = config.value("tileMinLod", 0);
    m_tileBudget = config.value("tileMaxTiles", 256);

    auto loadLayer = [&](const std::string& layerName, TileLayer& outLayer) -> bool {
        if (!manifest["layers"].contains(layerName)) {
            return false;
        }
        const auto& layer = manifest["layers"][layerName];
        outLayer.name = layerName;
        outLayer.format = layer.value("format", "png");
        outLayer.heightMin = layer.value("heightMin", 0.0f);
        outLayer.heightMax = layer.value("heightMax", 1.0f);
        if (!layer.contains("levels")) {
            return false;
        }
        for (const auto& entry : layer["levels"]) {
            TileLevel lvl;
            lvl.level = entry.value("level", 0);
            lvl.width = entry.value("width", 0);
            lvl.height = entry.value("height", 0);
            lvl.tilesX = entry.value("tilesX", 0);
            lvl.tilesY = entry.value("tilesY", 0);
            lvl.path = entry.value("path", "");
            if (entry.contains("pixelSize") && entry["pixelSize"].is_array() && entry["pixelSize"].size() == 2) {
                lvl.pixelSizeX = std::abs(entry["pixelSize"][0].get<float>());
                lvl.pixelSizeZ = std::abs(entry["pixelSize"][1].get<float>());
            }
            outLayer.levels.push_back(lvl);
        }
        return !outLayer.levels.empty();
    };

    std::string heightLayerName = config.value("tiledHeightLayer", "height");
    std::string albedoLayerName = config.value("tiledAlbedoLayer", "albedo");
    if (!loadLayer(heightLayerName, m_heightLayer)) {
        std::cerr << "Terrain manifest missing height layer: " << heightLayerName << "\n";
        return;
    }
    loadLayer(albedoLayerName, m_albedoLayer);

    const auto& level0 = m_heightLayer.levels.front();
    m_worldSizeX = level0.width * level0.pixelSizeX;
    m_worldSizeZ = level0.height * level0.pixelSizeZ;
    if (m_tileLodDistance <= 0.0f) {
        m_tileLodDistance = std::max(level0.pixelSizeX, level0.pixelSizeZ) * m_tileSizePx * 2.0f;
    }

    m_tileMaxLod = config.value("tileMaxLod", static_cast<int>(m_heightLayer.levels.size() - 1));
    m_tileMaxLod = std::clamp(m_tileMaxLod, 0, static_cast<int>(m_heightLayer.levels.size() - 1));
    m_tileMinLod = std::clamp(m_tileMinLod, 0, m_tileMaxLod);

    m_textured = !m_albedoLayer.levels.empty();
    m_tiled = true;
}

TerrainRenderer::TileResource* TerrainRenderer::ensureTileLoaded(const TileLevel& level, int x, int y) {
    if (!m_assets) {
        return nullptr;
    }
    std::string key = "L" + std::to_string(level.level) + "_x" + std::to_string(x) + "_y" + std::to_string(y);
    auto found = m_tileCache.find(key);
    if (found != m_tileCache.end()) {
        return &found->second;
    }

    if (level.tilesX <= 0 || level.tilesY <= 0) {
        return nullptr;
    }

    int tileWidthPx = std::max(0, std::min(m_tileSizePx, level.width - x * m_tileSizePx));
    int tileHeightPx = std::max(0, std::min(m_tileSizePx, level.height - y * m_tileSizePx));
    if (tileWidthPx == 0 || tileHeightPx == 0) {
        return nullptr;
    }

    float baseTileWorldX = m_tileSizePx * level.pixelSizeX;
    float baseTileWorldZ = m_tileSizePx * level.pixelSizeZ;
    float worldMinX = -m_worldSizeX * 0.5f;
    float worldMinZ = -m_worldSizeZ * 0.5f;
    float startX = worldMinX + x * baseTileWorldX;
    float startZ = worldMinZ + y * baseTileWorldZ;
    float tileSizeX = tileWidthPx * level.pixelSizeX;
    float tileSizeZ = tileHeightPx * level.pixelSizeZ;
    float offsetX = startX + tileSizeX * 0.5f;
    float offsetZ = startZ + tileSizeZ * 0.5f;

    std::filesystem::path heightPath = std::filesystem::path(m_manifestDir)
        / level.path / ("x" + std::to_string(x) + "_y" + std::to_string(y) + "." + m_heightLayer.format);
    if (!std::filesystem::exists(heightPath)) {
        return nullptr;
    }

    bool useTexture = false;
    std::filesystem::path albedoPath;
    if (!m_albedoLayer.levels.empty() && level.level < static_cast<int>(m_albedoLayer.levels.size())) {
        const auto& albedoLevel = m_albedoLayer.levels[level.level];
        albedoPath = std::filesystem::path(m_manifestDir)
            / albedoLevel.path / ("x" + std::to_string(x) + "_y" + std::to_string(y) + "." + m_albedoLayer.format);
        if (std::filesystem::exists(albedoPath)) {
            useTexture = true;
        }
    }

    int tileResolution = std::max(2, m_tileMaxResolution >> level.level);
    auto terrainData = MeshBuilder::terrainFromHeightmap(
        heightPath.string(),
        tileSizeX,
        tileSizeZ,
        m_heightLayer.heightMin,
        m_heightLayer.heightMax,
        tileResolution,
        useTexture,
        m_tileFlipY,
        offsetX,
        offsetZ
    );
    if (terrainData.empty()) {
        return nullptr;
    }

    std::string meshName = "terrain_tile_" + key;
    if (useTexture) {
        m_assets->loadTexturedMesh(meshName, terrainData);
    } else {
        m_assets->loadMesh(meshName, terrainData);
    }

    Texture* texture = nullptr;
    if (useTexture) {
        std::string texName = "terrain_tile_tex_" + key;
        if (m_assets->loadTexture(texName, albedoPath.string())) {
            texture = m_assets->getTexture(texName);
        }
    }

    TileResource resource;
    resource.mesh = m_assets->getMesh(meshName);
    resource.texture = texture;
    resource.center = Vec3(offsetX, 0.0f, offsetZ);
    resource.radius = std::max(tileSizeX, tileSizeZ) * 0.5f;
    resource.level = level.level;
    resource.x = x;
    resource.y = y;
    resource.textured = useTexture && texture != nullptr;

    auto inserted = m_tileCache.emplace(key, std::move(resource));
    return &inserted.first->second;
}

void TerrainRenderer::renderTiled(const Mat4& vp, const Vec3& sunDir, const Vec3& cameraPos) {
    if (m_heightLayer.levels.empty()) {
        return;
    }
    if (!m_shader) {
        m_shader = m_assets ? m_assets->getShader("basic") : nullptr;
    }
    if (!m_texturedShader) {
        m_texturedShader = m_assets ? m_assets->getShader("textured") : nullptr;
    }

    struct Candidate {
        const TileLevel* level = nullptr;
        int x = 0;
        int y = 0;
        float dist = 0.0f;
    };

    std::vector<Candidate> candidates;
    float worldMinX = -m_worldSizeX * 0.5f;
    float worldMinZ = -m_worldSizeZ * 0.5f;

    for (int levelIdx = m_tileMinLod; levelIdx <= m_tileMaxLod; ++levelIdx) {
        const auto& level = m_heightLayer.levels[levelIdx];
        if (level.tilesX <= 0 || level.tilesY <= 0) {
            continue;
        }
        float baseTileWorldX = m_tileSizePx * level.pixelSizeX;
        float baseTileWorldZ = m_tileSizePx * level.pixelSizeZ;

        float minDist = (levelIdx == m_tileMinLod) ? 0.0f : m_tileLodDistance * std::pow(2.0f, levelIdx - m_tileMinLod - 1);
        float maxDist = m_tileLodDistance * std::pow(2.0f, levelIdx - m_tileMinLod);

        float minX = cameraPos.x - m_tileViewDistance;
        float maxX = cameraPos.x + m_tileViewDistance;
        float minZ = cameraPos.z - m_tileViewDistance;
        float maxZ = cameraPos.z + m_tileViewDistance;

        int tx0 = static_cast<int>(std::floor((minX - worldMinX) / baseTileWorldX));
        int tx1 = static_cast<int>(std::floor((maxX - worldMinX) / baseTileWorldX));
        int ty0 = static_cast<int>(std::floor((minZ - worldMinZ) / baseTileWorldZ));
        int ty1 = static_cast<int>(std::floor((maxZ - worldMinZ) / baseTileWorldZ));

        tx0 = std::clamp(tx0, 0, level.tilesX - 1);
        tx1 = std::clamp(tx1, 0, level.tilesX - 1);
        ty0 = std::clamp(ty0, 0, level.tilesY - 1);
        ty1 = std::clamp(ty1, 0, level.tilesY - 1);

        for (int ty = ty0; ty <= ty1; ++ty) {
            for (int tx = tx0; tx <= tx1; ++tx) {
                float centerX = worldMinX + (tx + 0.5f) * baseTileWorldX;
                float centerZ = worldMinZ + (ty + 0.5f) * baseTileWorldZ;
                float dx = centerX - cameraPos.x;
                float dz = centerZ - cameraPos.z;
                float dist = std::sqrt(dx * dx + dz * dz);
                if (dist > m_tileViewDistance) {
                    continue;
                }
                if (dist < minDist || dist >= maxDist) {
                    continue;
                }
                candidates.push_back({&level, tx, ty, dist});
            }
        }
    }

    if (candidates.empty()) {
        return;
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.dist < b.dist;
    });

    int drawn = 0;
    for (const auto& candidate : candidates) {
        if (drawn >= m_tileBudget) {
            break;
        }
        TileResource* tile = ensureTileLoaded(*candidate.level, candidate.x, candidate.y);
        if (!tile || !tile->mesh) {
            continue;
        }

        Shader* shader = (tile->textured && m_texturedShader && tile->texture) ? m_texturedShader : m_shader;
        if (!shader) {
            continue;
        }

        shader->use();
        shader->setMat4("uMVP", vp);
        applyDirectionalLighting(shader, sunDir);
        if (tile->textured && tile->texture) {
            tile->texture->bind(0);
            shader->setInt("uTexture", 0);
        }
        tile->mesh->draw();
        drawn += 1;
    }
}

} // namespace nuage
