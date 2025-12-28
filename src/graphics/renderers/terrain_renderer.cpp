#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/mesh.hpp"
#include "graphics/mesh_builder.hpp"
#include "graphics/lighting.hpp"
#include "graphics/renderers/terrain/terrain_mask_blend.hpp"
#include "graphics/renderers/terrain/terrain_tile_io.hpp"
#include "utils/config_loader.hpp"
#include "math/vec2.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace nuage {

namespace {
constexpr int kMaxVisibleRadius = 8;
constexpr int kMaxLoadsPerFrame = 8;

bool buildGridVerticesFromTriList(const std::vector<float>& triVerts, int gridResolution,
                                  float tileMinX, float tileMinZ, float tileSize,
                                  std::vector<float>& outVerts) {
    if (gridResolution < 1 || tileSize <= 0.0f) {
        return false;
    }
    int res = gridResolution + 1;
    int stride = 9;
    size_t gridCount = static_cast<size_t>(res * res);
    outVerts.assign(gridCount * stride, 0.0f);
    std::vector<bool> filled(gridCount, false);

    size_t vertexCount = triVerts.size() / stride;
    for (size_t i = 0; i < vertexCount; ++i) {
        float px = triVerts[i * stride + 0];
        float pz = triVerts[i * stride + 2];
        float fx = (px - tileMinX) / tileSize;
        float fz = (pz - tileMinZ) / tileSize;
        int gx = static_cast<int>(std::lround(fx * (res - 1)));
        int gz = static_cast<int>(std::lround(fz * (res - 1)));
        gx = std::clamp(gx, 0, res - 1);
        gz = std::clamp(gz, 0, res - 1);
        size_t idx = static_cast<size_t>(gz * res + gx);
        size_t base = idx * stride;
        for (int c = 0; c < stride; ++c) {
            outVerts[base + c] = triVerts[i * stride + c];
        }
        filled[idx] = true;
    }

    for (bool ok : filled) {
        if (!ok) {
            return false;
        }
    }
    return true;
}

void buildGridIndices(int resX, int resZ, std::vector<std::uint32_t>& outIndices) {
    outIndices.clear();
    if (resX < 2 || resZ < 2) {
        return;
    }
    outIndices.reserve(static_cast<size_t>(resX - 1) * static_cast<size_t>(resZ - 1) * 6u);
    for (int z = 0; z < resZ - 1; ++z) {
        for (int x = 0; x < resX - 1; ++x) {
            std::uint32_t i00 = static_cast<std::uint32_t>(z * resX + x);
            std::uint32_t i10 = i00 + 1;
            std::uint32_t i01 = i00 + static_cast<std::uint32_t>(resX);
            std::uint32_t i11 = i01 + 1;
            outIndices.push_back(i00);
            outIndices.push_back(i10);
            outIndices.push_back(i11);
            outIndices.push_back(i00);
            outIndices.push_back(i11);
            outIndices.push_back(i01);
        }
    }
}

void buildLodVertices(const std::vector<float>& gridVerts, int resX, int resZ, int step,
                      std::vector<float>& outVerts) {
    int stride = 9;
    int lodResX = (resX - 1) / step + 1;
    int lodResZ = (resZ - 1) / step + 1;
    outVerts.assign(static_cast<size_t>(lodResX * lodResZ) * stride, 0.0f);
    for (int z = 0; z < lodResZ; ++z) {
        int srcZ = z * step;
        for (int x = 0; x < lodResX; ++x) {
            int srcX = x * step;
            size_t srcIdx = static_cast<size_t>(srcZ * resX + srcX) * stride;
            size_t dstIdx = static_cast<size_t>(z * lodResX + x) * stride;
            for (int c = 0; c < stride; ++c) {
                outVerts[dstIdx + c] = gridVerts[srcIdx + c];
            }
        }
    }
}

void buildLodIndices(int resX, int resZ, int step, std::vector<std::uint32_t>& outIndices) {
    int lodResX = (resX - 1) / step + 1;
    int lodResZ = (resZ - 1) / step + 1;
    buildGridIndices(lodResX, lodResZ, outIndices);
}

void addSkirt(std::vector<float>& verts, std::vector<std::uint32_t>& indices,
              int resX, int resZ, float depth) {
    if (resX < 2 || resZ < 2 || depth <= 0.0f) {
        return;
    }
    int stride = 9;
    std::vector<std::uint32_t> border;
    border.reserve(static_cast<size_t>((resX + resZ) * 2 - 4));

    for (int x = 0; x < resX; ++x) {
        border.push_back(static_cast<std::uint32_t>(x));
    }
    for (int z = 1; z < resZ; ++z) {
        border.push_back(static_cast<std::uint32_t>(z * resX + (resX - 1)));
    }
    for (int x = resX - 2; x >= 0; --x) {
        border.push_back(static_cast<std::uint32_t>((resZ - 1) * resX + x));
    }
    for (int z = resZ - 2; z >= 1; --z) {
        border.push_back(static_cast<std::uint32_t>(z * resX));
    }

    std::vector<std::uint32_t> skirt;
    skirt.reserve(border.size());
    for (std::uint32_t idx : border) {
        size_t base = static_cast<size_t>(idx) * stride;
        verts.push_back(verts[base + 0]);
        verts.push_back(verts[base + 1] - depth);
        verts.push_back(verts[base + 2]);
        verts.push_back(verts[base + 3]);
        verts.push_back(verts[base + 4]);
        verts.push_back(verts[base + 5]);
        verts.push_back(verts[base + 6]);
        verts.push_back(verts[base + 7]);
        verts.push_back(verts[base + 8]);
        skirt.push_back(static_cast<std::uint32_t>(verts.size() / stride - 1));
    }

    size_t count = border.size();
    for (size_t i = 0; i < count; ++i) {
        size_t next = (i + 1) % count;
        std::uint32_t b0 = border[i];
        std::uint32_t b1 = border[next];
        std::uint32_t s0 = skirt[i];
        std::uint32_t s1 = skirt[next];
        indices.push_back(b0);
        indices.push_back(b1);
        indices.push_back(s1);
        indices.push_back(b0);
        indices.push_back(s1);
        indices.push_back(s0);
    }
}
}

void TerrainRenderer::init(AssetStore& assets) {
    m_shader = assets.getShader("basic");
    m_texturedShader = assets.getShader("textured");
    m_assets = &assets;
}

void TerrainRenderer::shutdown() {
    m_tileCache.clear();
    m_procTileCreateCounts.clear();
    m_compiledTileCreateCounts.clear();
    m_compiledTiles.clear();
    m_procTilesLoadedThisFrame = 0;
    m_compiledTilesLoadedThisFrame = 0;
    m_procTileRebuilds = 0;
    m_compiledTileRebuilds = 0;
    m_mesh = nullptr;
    m_shader = nullptr;
    m_texturedShader = nullptr;
    m_texture = nullptr;
    m_textureSettings = TerrainTextureSettings{};
    m_texGrass = nullptr;
    m_texForest = nullptr;
    m_texRock = nullptr;
    m_texDirt = nullptr;
    m_texUrban = nullptr;
    m_assets = nullptr;
    m_textured = false;
    m_procedural = false;
    m_compiled = false;
}

void TerrainRenderer::setCompiledVisibleRadius(int radius) {
    m_compiledVisibleRadius = std::clamp(radius, 0, kMaxVisibleRadius);
}

void TerrainRenderer::setCompiledLoadsPerFrame(int loads) {
    m_compiledLoadsPerFrame = std::clamp(loads, 1, kMaxLoadsPerFrame);
}

void TerrainRenderer::setProceduralVisibleRadius(int radius) {
    m_procVisibleRadius = std::clamp(radius, 0, kMaxVisibleRadius);
}

void TerrainRenderer::setProceduralLoadsPerFrame(int loads) {
    m_procLoadsPerFrame = std::clamp(loads, 1, kMaxLoadsPerFrame);
}

void TerrainRenderer::setup(const std::string& configPath, AssetStore& assets) {
    m_assets = &assets;
    m_procedural = false;
    m_compiled = false;
    m_tileCache.clear();
    m_procTileCreateCounts.clear();
    m_procTileRebuilds = 0;
    m_compiledTileCreateCounts.clear();
    m_compiledTileRebuilds = 0;
    m_mesh = nullptr;
    m_texture = nullptr;
    m_textured = false;
    m_textureSettings = TerrainTextureSettings{};
    m_texGrass = nullptr;
    m_texForest = nullptr;
    m_texRock = nullptr;
    m_texDirt = nullptr;
    m_texUrban = nullptr;
    m_visuals.resetDefaults();

    if (configPath.empty()) {
        auto terrainData = MeshBuilder::terrain(20000.0f, 40);
        assets.loadMesh("session_terrain", terrainData);
        m_mesh = assets.getMesh("session_terrain");
        return;
    }

    auto terrainConfigOpt = loadJsonConfig(configPath);
    if (terrainConfigOpt && terrainConfigOpt->value("procedural", false)) {
        setupProcedural(configPath);
        if (m_procedural) {
            return;
        }
    }
    if (terrainConfigOpt && terrainConfigOpt->contains("compiledManifest")) {
        setupCompiled(configPath);
        if (m_compiled) {
            return;
        }
    }
    if (terrainConfigOpt && !m_procedural && !m_compiled) {
        std::cerr << "Unsupported terrain config; using flat terrain fallback.\n";
    }

    m_mesh = assets.getMesh("session_terrain");
}

void TerrainRenderer::render(const Mat4& vp, const Vec3& sunDir, const Vec3& cameraPos) {
    if (m_procedural) {
        renderProcedural(vp, sunDir, cameraPos);
        return;
    }
    if (m_compiled) {
        renderCompiled(vp, sunDir, cameraPos);
        return;
    }
    if (!m_mesh) return;

    Shader* shader = (m_textured && m_texturedShader && m_texture) ? m_texturedShader : m_shader;
    if (!shader) return;

    shader->use();
    shader->setMat4("uMVP", vp);
    applyDirectionalLighting(shader, sunDir);
    m_visuals.bind(shader, sunDir, cameraPos);
    if (m_textured && m_texture) {
        m_texture->bind(0);
        shader->setInt("uTexture", 0);
    }
    m_mesh->draw();
}

void TerrainRenderer::setupProcedural(const std::string& configPath) {
    auto terrainConfigOpt = loadJsonConfig(configPath);
    if (!terrainConfigOpt) {
        return;
    }
    const auto& config = *terrainConfigOpt;
    m_procTileSizeMeters = config.value("proceduralTileSize", 2000.0f);
    m_procGridResolution = config.value("proceduralGridResolution", 129);
    m_procVisibleRadius = config.value("proceduralVisibleRadius", 1);
    m_procLoadsPerFrame = config.value("proceduralMaxLoadsPerFrame", 2);
    m_procHeightAmplitude = config.value("proceduralHeightAmplitude", 250.0f);
    m_procHeightBase = config.value("proceduralHeightBase", 0.0f);
    m_procFrequency = config.value("proceduralFrequency", 0.0006f);
    m_procFrequency2 = config.value("proceduralFrequency2", 0.0013f);
    m_procSeed = config.value("proceduralSeed", 1337);
    m_procBorderWidth = config.value("proceduralBorderWidth", 0.03f);
    m_procDebugBorders = config.value("proceduralDebugBorders", true);
    m_procDebugLog = config.value("proceduralDebugLog", true);

    m_procGridResolution = std::max(2, m_procGridResolution);
    m_procVisibleRadius = std::max(0, m_procVisibleRadius);
    m_procLoadsPerFrame = std::max(1, m_procLoadsPerFrame);
    m_procTileSizeMeters = std::max(1.0f, m_procTileSizeMeters);
    m_procHeightAmplitude = std::max(0.0f, m_procHeightAmplitude);

    m_visuals.setHeightRange(m_procHeightBase - m_procHeightAmplitude,
                             m_procHeightBase + m_procHeightAmplitude);
    m_visuals.applyConfig(config);
    m_visuals.clamp();
    applyTextureConfig(config, configPath);

    m_textured = false;
    m_procedural = true;
}

void TerrainRenderer::setupCompiled(const std::string& configPath) {
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

    std::string manifestPath = resolve(config.value("compiledManifest", ""));
    if (manifestPath.empty()) {
        return;
    }

    auto manifestOpt = loadJsonConfig(manifestPath);
    if (!manifestOpt) {
        std::cerr << "Failed to load compiled terrain manifest: " << manifestPath << "\n";
        return;
    }
    const auto& manifest = *manifestOpt;

    m_compiledManifestDir = std::filesystem::path(manifestPath).parent_path().string();
    m_compiledTileSizeMeters = manifest.value("tileSizeMeters", 2000.0f);
    m_compiledGridResolution = manifest.value("gridResolution", 129);
    m_compiledMaskResolution = manifest.value("maskResolution", 0);
    if (manifest.contains("boundsENU") && manifest["boundsENU"].is_array() && manifest["boundsENU"].size() == 4) {
        m_compiledMinX = manifest["boundsENU"][0].get<float>();
        m_compiledMinZ = manifest["boundsENU"][1].get<float>();
        m_compiledMaxX = manifest["boundsENU"][2].get<float>();
        m_compiledMaxZ = manifest["boundsENU"][3].get<float>();
    }

    m_compiledVisibleRadius = config.value("compiledVisibleRadius", 1);
    m_compiledLoadsPerFrame = config.value("compiledMaxLoadsPerFrame", 2);
    m_compiledDebugLog = config.value("compiledDebugLog", true);
    m_compiledLod1Distance = config.value("compiledLod1Distance", m_compiledTileSizeMeters * 1.5f);
    m_compiledSkirtDepth = config.value("compiledSkirtDepth", m_compiledTileSizeMeters * 0.05f);

    m_compiledTileSizeMeters = std::max(1.0f, m_compiledTileSizeMeters);
    m_compiledGridResolution = std::max(2, m_compiledGridResolution);
    m_compiledVisibleRadius = std::max(0, m_compiledVisibleRadius);
    m_compiledLoadsPerFrame = std::max(1, m_compiledLoadsPerFrame);
    m_compiledLod1Distance = std::max(0.0f, m_compiledLod1Distance);
    m_compiledLod1DistanceSq = m_compiledLod1Distance * m_compiledLod1Distance;
    m_compiledSkirtDepth = std::max(0.0f, m_compiledSkirtDepth);

    m_compiledTiles.clear();
    if (manifest.contains("tileIndex") && manifest["tileIndex"].is_array()) {
        for (const auto& entry : manifest["tileIndex"]) {
            if (!entry.is_array() || entry.size() != 2) {
                continue;
            }
            int tx = entry[0].get<int>();
            int ty = entry[1].get<int>();
            m_compiledTiles.insert(packedTileKey(tx, ty));
        }
    }

    if (m_compiledTiles.empty()) {
        std::cerr << "Compiled terrain manifest has no tiles listed: " << manifestPath << "\n";
        return;
    }

    m_visuals.applyConfig(config);
    m_visuals.clamp();
    applyTextureConfig(config, configPath);

    m_textured = false;
    m_compiled = true;
}

void TerrainRenderer::applyTextureConfig(const nlohmann::json& config, const std::string& configPath) {
    m_textureSettings = TerrainTextureSettings{};
    if (!config.contains("terrainTextures") || !config["terrainTextures"].is_object()) {
        m_textureSettings.enabled = false;
        return;
    }

    const auto& texConfig = config["terrainTextures"];
    m_textureSettings.enabled = texConfig.value("enabled", true);
    m_textureSettings.texScale = texConfig.value("texScale", m_textureSettings.texScale);
    m_textureSettings.detailScale = texConfig.value("detailScale", m_textureSettings.detailScale);
    m_textureSettings.detailStrength = texConfig.value("detailStrength", m_textureSettings.detailStrength);
    m_textureSettings.rockSlopeStart = texConfig.value("rockSlopeStart", m_textureSettings.rockSlopeStart);
    m_textureSettings.rockSlopeEnd = texConfig.value("rockSlopeEnd", m_textureSettings.rockSlopeEnd);
    m_textureSettings.rockStrength = texConfig.value("rockStrength", m_textureSettings.rockStrength);
    if (texConfig.contains("waterColor") && texConfig["waterColor"].is_array()
        && texConfig["waterColor"].size() == 3) {
        m_textureSettings.waterColor = Vec3(texConfig["waterColor"][0].get<float>(),
                                            texConfig["waterColor"][1].get<float>(),
                                            texConfig["waterColor"][2].get<float>());
    }

    if (!m_assets || !m_textureSettings.enabled) {
        m_textureSettings.enabled = false;
        return;
    }

    std::filesystem::path cfg(configPath);
    auto resolve = [&](const std::string& p) {
        if (p.empty()) return p;
        std::filesystem::path path(p);
        if (path.is_absolute()) return p;
        return (cfg.parent_path() / path).string();
    };

    auto loadTex = [&](const char* key, Texture*& out, const char* name) -> bool {
        if (!texConfig.contains(key)) {
            return false;
        }
        std::string path = texConfig.value(key, "");
        if (path.empty()) {
            return false;
        }
        std::string resolved = resolve(path);
        if (!m_assets->loadTexture(name, resolved, true)) {
            std::cerr << "[terrain] failed to load texture " << resolved << "\n";
            return false;
        }
        out = m_assets->getTexture(name);
        return out != nullptr;
    };

    bool loadedAny = false;
    loadedAny |= loadTex("grass", m_texGrass, "terrain_grass");
    loadedAny |= loadTex("forest", m_texForest, "terrain_forest");
    loadedAny |= loadTex("rock", m_texRock, "terrain_rock");
    loadedAny |= loadTex("dirt", m_texDirt, "terrain_dirt");
    loadedAny |= loadTex("urban", m_texUrban, "terrain_urban");
    if (!loadedAny) {
        m_textureSettings.enabled = false;
    }
}

void TerrainRenderer::bindTerrainTextures(Shader* shader, bool useMasks) const {
    if (!shader) {
        return;
    }

    Texture* grass = m_texGrass;
    Texture* forest = m_texForest ? m_texForest : grass;
    Texture* rock = m_texRock ? m_texRock : grass;
    Texture* dirt = m_texDirt ? m_texDirt : grass;
    Texture* urban = m_texUrban ? m_texUrban : (dirt ? dirt : grass);
    bool enabled = m_textureSettings.enabled && grass && rock && urban;
    shader->setBool("uTerrainUseTextures", enabled);
    shader->setBool("uTerrainUseMasks", useMasks);
    if (!enabled) {
        return;
    }

    shader->setFloat("uTerrainTexScale", m_textureSettings.texScale);
    shader->setFloat("uTerrainDetailScale", m_textureSettings.detailScale);
    shader->setFloat("uTerrainDetailStrength", m_textureSettings.detailStrength);
    shader->setFloat("uTerrainRockSlopeStart", m_textureSettings.rockSlopeStart);
    shader->setFloat("uTerrainRockSlopeEnd", m_textureSettings.rockSlopeEnd);
    shader->setFloat("uTerrainRockStrength", m_textureSettings.rockStrength);
    shader->setVec3("uTerrainWaterColor", m_textureSettings.waterColor);

    grass->bind(0);
    shader->setInt("uTerrainTexGrass", 0);
    forest->bind(1);
    shader->setInt("uTerrainTexForest", 1);
    rock->bind(2);
    shader->setInt("uTerrainTexRock", 2);
    dirt->bind(3);
    shader->setInt("uTerrainTexDirt", 3);
    urban->bind(4);
    shader->setInt("uTerrainTexUrban", 4);
}

namespace {
Vec3 heightColorLocal(float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    if (t < 0.3f) {
        float k = t / 0.3f;
        return Vec3(0.15f + 0.1f * k, 0.35f + 0.3f * k, 0.15f + 0.1f * k);
    }
    if (t < 0.7f) {
        float k = (t - 0.3f) / 0.4f;
        return Vec3(0.25f + 0.25f * k, 0.55f - 0.15f * k, 0.2f + 0.1f * k);
    }
    float k = (t - 0.7f) / 0.3f;
    return Vec3(0.55f + 0.35f * k, 0.5f + 0.35f * k, 0.45f + 0.3f * k);
}
}

float TerrainRenderer::proceduralHeight(float worldX, float worldZ) const {
    float phase = static_cast<float>(m_procSeed) * 0.031f;
    float h1 = std::sin((worldX + phase) * m_procFrequency) * std::cos((worldZ - phase) * m_procFrequency);
    float h2 = std::sin((worldX + phase * 2.0f) * m_procFrequency2 + std::cos(worldZ * m_procFrequency2));
    float h3 = std::sin((worldX - phase) * (m_procFrequency * 0.5f)) * std::sin((worldZ + phase) * (m_procFrequency * 0.5f));
    float combined = h1 * 0.6f + h2 * 0.25f + h3 * 0.15f;
    return m_procHeightBase + combined * m_procHeightAmplitude;
}

Vec3 TerrainRenderer::proceduralTileTint(int tileX, int tileY) const {
    std::uint32_t ux = static_cast<std::uint32_t>(tileX);
    std::uint32_t uy = static_cast<std::uint32_t>(tileY);
    std::uint32_t seed = static_cast<std::uint32_t>(m_procSeed);
    std::uint32_t hash = (ux * 73856093u) ^ (uy * 19349663u) ^ (seed * 83492791u);
    float r = 0.6f + 0.4f * ((hash & 0xFFu) / 255.0f);
    float g = 0.6f + 0.4f * (((hash >> 8) & 0xFFu) / 255.0f);
    float b = 0.6f + 0.4f * (((hash >> 16) & 0xFFu) / 255.0f);
    return Vec3(r, g, b);
}

std::int64_t TerrainRenderer::packedTileKey(int x, int y) const {
    return (static_cast<std::int64_t>(x) << 32) ^ (static_cast<std::uint32_t>(y));
}


TerrainRenderer::TileResource* TerrainRenderer::ensureProceduralTileLoaded(int x, int y) {
    std::string key = "P_x" + std::to_string(x) + "_y" + std::to_string(y);
    auto found = m_tileCache.find(key);
    if (found != m_tileCache.end()) {
        return &found->second;
    }
    if (m_procTilesLoadedThisFrame >= m_procLoadsPerFrame) {
        return nullptr;
    }

    int cells = m_procGridResolution;
    int resX = cells + 1;
    int resZ = cells + 1;
    float tileSize = m_procTileSizeMeters;
    float originX = static_cast<float>(x) * tileSize;
    float originZ = static_cast<float>(y) * tileSize;

    std::vector<float> verts;
    int stride = 9;
    verts.reserve((resX - 1) * (resZ - 1) * 6 * stride);

    std::vector<Vec3> positions(resX * resZ);
    std::vector<Vec3> normals(resX * resZ, Vec3(0, 1, 0));

    float heightMin = m_procHeightBase - m_procHeightAmplitude;
    float heightMax = m_procHeightBase + m_procHeightAmplitude;
    float heightRange = std::max(1.0f, heightMax - heightMin);
    Vec3 tileTint = proceduralTileTint(x, y);

    for (int z = 0; z < resZ; ++z) {
        for (int xIdx = 0; xIdx < resX; ++xIdx) {
            float fx = (resX > 1) ? static_cast<float>(xIdx) / (resX - 1) : 0.0f;
            float fz = (resZ > 1) ? static_cast<float>(z) / (resZ - 1) : 0.0f;

            float worldX = originX + fx * tileSize;
            float worldZ = originZ + fz * tileSize;
            float worldY = proceduralHeight(worldX, worldZ);

            int idx = z * resX + xIdx;
            positions[idx] = Vec3(worldX, worldY, worldZ);
        }
    }

    for (int z = 0; z < resZ; ++z) {
        for (int xIdx = 0; xIdx < resX; ++xIdx) {
            int idx = z * resX + xIdx;
            int left = z * resX + std::max(xIdx - 1, 0);
            int right = z * resX + std::min(xIdx + 1, resX - 1);
            int up = std::max(z - 1, 0) * resX + xIdx;
            int down = std::min(z + 1, resZ - 1) * resX + xIdx;

            Vec3 tangentX = positions[right] - positions[left];
            Vec3 tangentZ = positions[down] - positions[up];
            Vec3 normal = tangentZ.cross(tangentX);
            normals[idx] = (normal.length() > 1e-6f) ? normal.normalized() : Vec3(0, 1, 0);
        }
    }

    auto appendVertex = [&](int vertexIdx) {
        const Vec3& pos = positions[vertexIdx];
        const Vec3& normal = normals[vertexIdx];
        float t = (pos.y - heightMin) / heightRange;
        t = std::clamp(t, 0.0f, 1.0f);
        Vec3 baseColor = heightColorLocal(t);

        Vec3 color = Vec3(baseColor.x * tileTint.x, baseColor.y * tileTint.y, baseColor.z * tileTint.z);
        verts.insert(verts.end(), {
            pos.x, pos.y, pos.z,
            normal.x, normal.y, normal.z,
            color.x, color.y, color.z
        });
    };

    for (int z = 0; z < resZ - 1; ++z) {
        for (int xIdx = 0; xIdx < resX - 1; ++xIdx) {
            int i00 = z * resX + xIdx;
            int i10 = i00 + 1;
            int i01 = i00 + resX;
            int i11 = i01 + 1;

            appendVertex(i00);
            appendVertex(i10);
            appendVertex(i11);

            appendVertex(i00);
            appendVertex(i11);
            appendVertex(i01);
        }
    }

    if (m_procDebugBorders) {
        float border = std::clamp(m_procBorderWidth, 0.0f, 0.49f);
        Vec3 borderColor(0.85f, 0.1f, 0.1f);
        size_t vertexCount = verts.size() / stride;
        for (size_t i = 0; i < vertexCount; ++i) {
            float px = verts[i * stride + 0];
            float pz = verts[i * stride + 2];
            float fx = (px - originX) / tileSize;
            float fz = (pz - originZ) / tileSize;
            if (fx <= border || fx >= 1.0f - border || fz <= border || fz >= 1.0f - border) {
                verts[i * stride + 6] = borderColor.x;
                verts[i * stride + 7] = borderColor.y;
                verts[i * stride + 8] = borderColor.z;
            }
        }
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->init(verts);
    m_procTilesLoadedThisFrame += 1;

    TileResource resource;
    resource.ownedMesh = std::move(mesh);
    resource.mesh = resource.ownedMesh.get();
    resource.texture = nullptr;
    resource.center = Vec3(originX + tileSize * 0.5f, 0.0f, originZ + tileSize * 0.5f);
    resource.radius = tileSize * 0.5f;
    resource.level = 0;
    resource.x = x;
    resource.y = y;
    resource.textured = false;
    resource.procedural = true;

    auto inserted = m_tileCache.emplace(key, std::move(resource));
    if (m_procDebugLog) {
        int& count = m_procTileCreateCounts[key];
        count += 1;
        if (count > 1) {
            m_procTileRebuilds += 1;
            std::cout << "[terrain] procedural tile rebuilt " << x << "," << y
                      << " total_rebuilds=" << m_procTileRebuilds << "\n";
        }
        std::cout << "[terrain] loaded procedural tile " << x << "," << y << "\n";
    }
    return &inserted.first->second;
}

TerrainRenderer::TileResource* TerrainRenderer::ensureCompiledTileLoaded(int x, int y) {
    if (!m_assets) {
        return nullptr;
    }
    if (m_compiledTiles.find(packedTileKey(x, y)) == m_compiledTiles.end()) {
        return nullptr;
    }

    std::string key = "C_x" + std::to_string(x) + "_y" + std::to_string(y);
    auto found = m_tileCache.find(key);
    if (found != m_tileCache.end()) {
        return &found->second;
    }
    if (m_compiledTilesLoadedThisFrame >= m_compiledLoadsPerFrame) {
        return nullptr;
    }

    std::filesystem::path meshPath = std::filesystem::path(m_compiledManifestDir)
        / "tiles" / ("tile_" + std::to_string(x) + "_" + std::to_string(y) + ".mesh");

    std::vector<float> verts;
    if (!load_compiled_mesh(meshPath.string(), verts)) {
        if (m_compiledDebugLog) {
            std::cout << "[terrain] missing compiled tile " << x << "," << y << "\n";
        }
        return nullptr;
    }

    float tileMinX = static_cast<float>(x) * m_compiledTileSizeMeters;
    float tileMinZ = static_cast<float>(y) * m_compiledTileSizeMeters;
    if (m_compiledMaskResolution > 0) {
        std::filesystem::path maskPath = std::filesystem::path(m_compiledManifestDir)
            / "tiles" / ("tile_" + std::to_string(x) + "_" + std::to_string(y) + ".mask");
        std::vector<std::uint8_t> mask;
        if (load_compiled_mask(maskPath.string(), m_compiledMaskResolution, mask)) {
            apply_mask_to_verts(verts, mask, m_compiledMaskResolution, m_compiledTileSizeMeters, tileMinX, tileMinZ);
        }
    }

    auto mesh = std::make_unique<Mesh>();
    std::vector<float> gridVerts;
    std::vector<std::uint32_t> indices;
    bool builtGrid = buildGridVerticesFromTriList(verts, m_compiledGridResolution,
                                                  tileMinX, tileMinZ, m_compiledTileSizeMeters,
                                                  gridVerts);
    if (builtGrid) {
        int res = m_compiledGridResolution + 1;
        buildGridIndices(res, res, indices);
        addSkirt(gridVerts, indices, res, res, m_compiledSkirtDepth);
        mesh->initIndexed(gridVerts, indices);
    } else {
        mesh->init(verts);
    }
    m_compiledTilesLoadedThisFrame += 1;

    TileResource resource;
    resource.ownedMesh = std::move(mesh);
    resource.mesh = resource.ownedMesh.get();
    resource.ownedMeshLod1 = nullptr;
    resource.meshLod1 = nullptr;
    resource.texture = nullptr;
    resource.center = Vec3((static_cast<float>(x) + 0.5f) * m_compiledTileSizeMeters,
                           0.0f,
                           (static_cast<float>(y) + 0.5f) * m_compiledTileSizeMeters);
    resource.radius = m_compiledTileSizeMeters * 0.5f;
    resource.level = 0;
    resource.x = x;
    resource.y = y;
    resource.textured = false;
    resource.procedural = false;
    resource.compiled = true;

    if (builtGrid && m_compiledGridResolution >= 2) {
        int res = m_compiledGridResolution + 1;
        std::vector<float> lodVerts;
        std::vector<std::uint32_t> lodIndices;
        buildLodVertices(gridVerts, res, res, 2, lodVerts);
        buildLodIndices(res, res, 2, lodIndices);
        addSkirt(lodVerts, lodIndices, (res - 1) / 2 + 1, (res - 1) / 2 + 1, m_compiledSkirtDepth);
        if (!lodVerts.empty() && !lodIndices.empty()) {
            auto lodMesh = std::make_unique<Mesh>();
            lodMesh->initIndexed(lodVerts, lodIndices);
            resource.ownedMeshLod1 = std::move(lodMesh);
            resource.meshLod1 = resource.ownedMeshLod1.get();
        }
    }

    auto inserted = m_tileCache.emplace(key, std::move(resource));
    if (m_compiledDebugLog) {
        int& count = m_compiledTileCreateCounts[key];
        count += 1;
        if (count > 1) {
            m_compiledTileRebuilds += 1;
            std::cout << "[terrain] compiled tile rebuilt " << x << "," << y
                      << " total_rebuilds=" << m_compiledTileRebuilds << "\n";
        }
        std::cout << "[terrain] loaded compiled tile " << x << "," << y << "\n";
    }
    return &inserted.first->second;
}

void TerrainRenderer::renderProcedural(const Mat4& vp, const Vec3& sunDir, const Vec3& cameraPos) {
    if (!m_shader) {
        m_shader = m_assets ? m_assets->getShader("basic") : nullptr;
    }
    if (!m_shader) {
        return;
    }

    m_procTilesLoadedThisFrame = 0;

    int centerX = static_cast<int>(std::floor(cameraPos.x / m_procTileSizeMeters));
    int centerY = static_cast<int>(std::floor(cameraPos.z / m_procTileSizeMeters));

    std::unordered_set<std::string> desiredKeys;
    desiredKeys.reserve(static_cast<size_t>((m_procVisibleRadius * 2 + 1) * (m_procVisibleRadius * 2 + 1)));

    for (int dy = -m_procVisibleRadius; dy <= m_procVisibleRadius; ++dy) {
        for (int dx = -m_procVisibleRadius; dx <= m_procVisibleRadius; ++dx) {
            int tx = centerX + dx;
            int ty = centerY + dy;
            std::string key = "P_x" + std::to_string(tx) + "_y" + std::to_string(ty);
            desiredKeys.insert(key);
            TileResource* tile = ensureProceduralTileLoaded(tx, ty);
            if (!tile || !tile->mesh) {
                continue;
            }

            m_shader->use();
            m_shader->setMat4("uMVP", vp);
            applyDirectionalLighting(m_shader, sunDir);
            m_visuals.bind(m_shader, sunDir, cameraPos);
            bindTerrainTextures(m_shader, false);
            tile->mesh->draw();
        }
    }

    if (!m_tileCache.empty()) {
        std::vector<std::string> toRemove;
        toRemove.reserve(m_tileCache.size());
        for (const auto& pair : m_tileCache) {
            if (!pair.second.procedural) {
                continue;
            }
            if (desiredKeys.find(pair.first) == desiredKeys.end()) {
                toRemove.push_back(pair.first);
            }
        }
        for (const auto& key : toRemove) {
            auto it = m_tileCache.find(key);
            if (it == m_tileCache.end()) {
                continue;
            }
            if (m_procDebugLog) {
                std::cout << "[terrain] unloaded procedural tile " << it->second.x << "," << it->second.y << "\n";
            }
            m_tileCache.erase(it);
        }
    }
}

void TerrainRenderer::renderCompiled(const Mat4& vp, const Vec3& sunDir, const Vec3& cameraPos) {
    if (!m_shader) {
        m_shader = m_assets ? m_assets->getShader("basic") : nullptr;
    }
    if (!m_shader) {
        return;
    }

    m_compiledTilesLoadedThisFrame = 0;

    int centerX = static_cast<int>(std::floor(cameraPos.x / m_compiledTileSizeMeters));
    int centerY = static_cast<int>(std::floor(cameraPos.z / m_compiledTileSizeMeters));

    std::unordered_set<std::string> desiredKeys;
    desiredKeys.reserve(static_cast<size_t>((m_compiledVisibleRadius * 2 + 1) * (m_compiledVisibleRadius * 2 + 1)));

    for (int dy = -m_compiledVisibleRadius; dy <= m_compiledVisibleRadius; ++dy) {
        for (int dx = -m_compiledVisibleRadius; dx <= m_compiledVisibleRadius; ++dx) {
            int tx = centerX + dx;
            int ty = centerY + dy;
            if (m_compiledTiles.find(packedTileKey(tx, ty)) == m_compiledTiles.end()) {
                continue;
            }

            std::string key = "C_x" + std::to_string(tx) + "_y" + std::to_string(ty);
            desiredKeys.insert(key);

            TileResource* tile = ensureCompiledTileLoaded(tx, ty);
            if (!tile || !tile->mesh) {
                continue;
            }

            m_shader->use();
            m_shader->setMat4("uMVP", vp);
            applyDirectionalLighting(m_shader, sunDir);
            m_visuals.bind(m_shader, sunDir, cameraPos);
            bindTerrainTextures(m_shader, m_compiledMaskResolution > 0);
            Mesh* meshToDraw = tile->mesh;
            if (tile->meshLod1 && m_compiledLod1DistanceSq > 0.0f) {
                float dx = tile->center.x - cameraPos.x;
                float dz = tile->center.z - cameraPos.z;
                float distSq = dx * dx + dz * dz;
                if (distSq >= m_compiledLod1DistanceSq) {
                    meshToDraw = tile->meshLod1;
                }
            }
            meshToDraw->draw();
        }
    }

    if (!m_tileCache.empty()) {
        std::vector<std::string> toRemove;
        toRemove.reserve(m_tileCache.size());
        for (const auto& pair : m_tileCache) {
            if (!pair.second.compiled) {
                continue;
            }
            if (desiredKeys.find(pair.first) == desiredKeys.end()) {
                toRemove.push_back(pair.first);
            }
        }
        for (const auto& key : toRemove) {
            auto it = m_tileCache.find(key);
            if (it == m_tileCache.end()) {
                continue;
            }
            if (m_compiledDebugLog) {
                std::cout << "[terrain] unloaded compiled tile " << it->second.x << "," << it->second.y << "\n";
            }
            m_tileCache.erase(it);
        }
    }
}

} // namespace nuage
