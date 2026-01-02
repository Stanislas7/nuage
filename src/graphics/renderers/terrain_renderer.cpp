#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/lighting.hpp"
#include "graphics/mesh.hpp"
#include "graphics/mesh_builder.hpp"
#include "graphics/shader.hpp"
#include "utils/config_loader.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace nuage {

namespace {
constexpr int kMaxVisibleRadius = 8;
constexpr int kMaxLoadsPerFrame = 8;
}

void TerrainRenderer::init(AssetStore& assets) {
    m_shader = assets.getShader("basic");
    m_texturedShader = assets.getShader("textured");
    m_assets = &assets;
}

void TerrainRenderer::shutdown() {
    m_tileCache.clear();
    m_compiledTileCreateCounts.clear();
    m_compiledTiles.clear();
    m_compiledTilesLoadedThisFrame = 0;
    m_compiledTileRebuilds = 0;
    m_mesh = nullptr;
    m_shader = nullptr;
    m_texturedShader = nullptr;
    m_textureSettings = TerrainTextureSettings{};
    m_texGrass = nullptr;
    m_texForest = nullptr;
    m_texRock = nullptr;
    m_texDirt = nullptr;
    m_texUrban = nullptr;
    m_assets = nullptr;
    m_compiled = false;
    m_debugMaskView = false;
}

void TerrainRenderer::setCompiledVisibleRadius(int radius) {
    m_compiledVisibleRadius = std::clamp(radius, 0, kMaxVisibleRadius);
}

void TerrainRenderer::setCompiledLoadsPerFrame(int loads) {
    m_compiledLoadsPerFrame = std::clamp(loads, 1, kMaxLoadsPerFrame);
}

void TerrainRenderer::setTreesEnabled(bool enabled) {
    if (m_treesEnabled == enabled) {
        return;
    }
    m_treesEnabled = enabled;
    if (!m_tileCache.empty()) {
        m_tileCache.clear();
        m_compiledTileCreateCounts.clear();
        m_compiledTileRebuilds = 0;
    }
}

void TerrainRenderer::setup(const std::string& configPath, AssetStore& assets) {
    m_assets = &assets;
    m_compiled = false;
    m_tileCache.clear();
    m_compiledTileCreateCounts.clear();
    m_compiledTileRebuilds = 0;
    m_mesh = nullptr;
    m_textureSettings = TerrainTextureSettings{};
    m_texGrass = nullptr;
    m_texForest = nullptr;
    m_texRock = nullptr;
    m_texDirt = nullptr;
    m_texUrban = nullptr;
    m_visuals.resetDefaults();
    m_treesEnabled = false;
    m_debugMaskView = false;
    m_treesDensityPerSqKm = 80.0f;
    m_treesMinHeight = 4.0f;
    m_treesMaxHeight = 10.0f;
    m_treesMinRadius = 0.8f;
    m_treesMaxRadius = 2.2f;
    m_treesMaxSlope = 0.7f;
    m_treesMaxDistance = 5000.0f;
    m_treesMaxDistanceSq = m_treesMaxDistance * m_treesMaxDistance;
    m_treesSeed = 1337;

    if (configPath.empty()) {
        auto terrainData = MeshBuilder::terrain(20000.0f, 40);
        assets.loadMesh("session_terrain", terrainData);
        m_mesh = assets.getMesh("session_terrain");
        return;
    }

    auto terrainConfigOpt = loadJsonConfig(configPath);
    if (terrainConfigOpt && terrainConfigOpt->contains("compiledManifest")) {
        setupCompiled(configPath);
        if (m_compiled) {
            return;
        }
    }
    if (terrainConfigOpt && !m_compiled) {
        std::cerr << "Unsupported terrain config; using flat terrain fallback.\n";
    }

    m_mesh = assets.getMesh("session_terrain");
}

void TerrainRenderer::render(const Mat4& vp, const Vec3& sunDir, const Vec3& cameraPos) {
    if (m_compiled) {
        renderCompiled(vp, sunDir, cameraPos);
        return;
    }
    if (!m_mesh) return;

    if (!m_shader) return;

    m_shader->use();
    m_shader->setMat4("uMVP", vp);
    applyDirectionalLighting(m_shader, sunDir);
    m_visuals.bind(m_shader, sunDir, cameraPos);
    m_mesh->draw();
}

bool TerrainRenderer::sampleSurface(float worldX, float worldZ, TerrainSample& outSample) const {
    outSample = TerrainSample{};

    if (sampleRunway(worldX, worldZ, outSample)) {
        return true;
    }

    if (!m_compiled) {
        return false;
    }

    int tx = static_cast<int>(std::floor(worldX / m_compiledTileSizeMeters));
    int ty = static_cast<int>(std::floor(worldZ / m_compiledTileSizeMeters));
    return sampleCompiledSurface(tx, ty, worldX, worldZ, true, outSample);
}

bool TerrainRenderer::sampleSurfaceNoLoad(float worldX, float worldZ, TerrainSample& outSample) const {
    outSample = TerrainSample{};

    if (sampleRunway(worldX, worldZ, outSample)) {
        return true;
    }

    if (!m_compiled) {
        return false;
    }

    int tx = static_cast<int>(std::floor(worldX / m_compiledTileSizeMeters));
    int ty = static_cast<int>(std::floor(worldZ / m_compiledTileSizeMeters));
    return sampleCompiledSurfaceCached(tx, ty, worldX, worldZ, outSample);
}

bool TerrainRenderer::sampleHeight(float worldX, float worldZ, float& outHeight) const {
    TerrainSample sample;
    if (!sampleSurface(worldX, worldZ, sample)) {
        return false;
    }
    outHeight = sample.height;
    return true;
}

bool TerrainRenderer::sampleSurfaceHeight(float worldX, float worldZ, float& outHeight) const {
    TerrainSample sample;
    if (!sampleSurface(worldX, worldZ, sample)) {
        return false;
    }
    outHeight = sample.height;
    return true;
}

bool TerrainRenderer::sampleSurfaceHeightNoLoad(float worldX, float worldZ, float& outHeight) const {
    TerrainSample sample;
    if (!sampleSurfaceNoLoad(worldX, worldZ, sample)) {
        return false;
    }
    outHeight = sample.height;
    return true;
}

void TerrainRenderer::preloadPhysicsAt(float worldX, float worldZ, int radius) {
    if (!m_compiled) {
        return;
    }
    int cx = static_cast<int>(std::floor(worldX / m_compiledTileSizeMeters));
    int cy = static_cast<int>(std::floor(worldZ / m_compiledTileSizeMeters));
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            ensureCompiledTileLoaded(cx + dx, cy + dy, true);
        }
    }
}

} // namespace nuage
