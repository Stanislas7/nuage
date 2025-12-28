#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/mesh.hpp"
#include "graphics/mesh_builder.hpp"
#include "graphics/lighting.hpp"
#include "utils/config_loader.hpp"
#include "math/vec2.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

namespace nuage {

namespace {
Vec3 terrainFogColor(const Vec3& sunDir);
float smoothstepLocal(float edge0, float edge1, float x);
}

void TerrainRenderer::init(AssetStore& assets) {
    m_shader = assets.getShader("basic");
    m_texturedShader = assets.getShader("textured");
    m_assets = &assets;
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
    m_terrainHeightMin = 0.0f;
    m_terrainHeightMax = 1500.0f;
    m_terrainNoiseScale = 0.002f;
    m_terrainNoiseStrength = 0.3f;
    m_terrainSlopeStart = 0.3f;
    m_terrainSlopeEnd = 0.7f;
    m_terrainSlopeDarken = 0.3f;
    m_terrainFogDistance = 12000.0f;
    m_terrainDesaturate = 0.2f;
    m_terrainTint = Vec3(0.45f, 0.52f, 0.33f);
    m_terrainTintStrength = 0.15f;
    m_terrainDistanceDesatStart = 3000.0f;
    m_terrainDistanceDesatEnd = 12000.0f;
    m_terrainDistanceDesatStrength = 0.35f;
    m_terrainDistanceContrastLoss = 0.25f;
    m_terrainFogSunScale = 0.35f;

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
    shader->setBool("uTerrainShading", true);
    shader->setVec3("uCameraPos", cameraPos);
    shader->setVec3("uTerrainFogColor", terrainFogColor(sunDir));
    float elevation = std::clamp(sunDir.normalized().y, -1.0f, 1.0f);
    float dayFactor = smoothstepLocal(-0.25f, 0.35f, elevation);
    float fogScale = 1.0f - m_terrainFogSunScale * (1.0f - dayFactor);
    float fogDistance = m_terrainFogDistance * std::max(0.15f, fogScale);
    shader->setFloat("uTerrainHeightMin", m_terrainHeightMin);
    shader->setFloat("uTerrainHeightMax", m_terrainHeightMax);
    shader->setFloat("uTerrainNoiseScale", m_terrainNoiseScale);
    shader->setFloat("uTerrainNoiseStrength", m_terrainNoiseStrength);
    shader->setFloat("uTerrainSlopeStart", m_terrainSlopeStart);
    shader->setFloat("uTerrainSlopeEnd", m_terrainSlopeEnd);
    shader->setFloat("uTerrainSlopeDarken", m_terrainSlopeDarken);
    shader->setFloat("uTerrainFogDistance", fogDistance);
    shader->setFloat("uTerrainDesaturate", m_terrainDesaturate);
    shader->setVec3("uTerrainTint", m_terrainTint);
    shader->setFloat("uTerrainTintStrength", m_terrainTintStrength);
    shader->setFloat("uTerrainDistanceDesatStart", m_terrainDistanceDesatStart);
    shader->setFloat("uTerrainDistanceDesatEnd", m_terrainDistanceDesatEnd);
    shader->setFloat("uTerrainDistanceDesatStrength", m_terrainDistanceDesatStrength);
    shader->setFloat("uTerrainDistanceContrastLoss", m_terrainDistanceContrastLoss);
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

    m_terrainHeightMin = m_procHeightBase - m_procHeightAmplitude;
    m_terrainHeightMax = m_procHeightBase + m_procHeightAmplitude;
    if (config.contains("terrainVisuals") && config["terrainVisuals"].is_object()) {
        const auto& visuals = config["terrainVisuals"];
        m_terrainHeightMin = visuals.value("heightMin", m_terrainHeightMin);
        m_terrainHeightMax = visuals.value("heightMax", m_terrainHeightMax);
        m_terrainNoiseScale = visuals.value("noiseScale", m_terrainNoiseScale);
        m_terrainNoiseStrength = visuals.value("noiseStrength", m_terrainNoiseStrength);
        m_terrainSlopeStart = visuals.value("slopeStart", m_terrainSlopeStart);
        m_terrainSlopeEnd = visuals.value("slopeEnd", m_terrainSlopeEnd);
        m_terrainSlopeDarken = visuals.value("slopeDarken", m_terrainSlopeDarken);
        m_terrainFogDistance = visuals.value("fogDistance", m_terrainFogDistance);
        m_terrainDesaturate = visuals.value("desaturate", m_terrainDesaturate);
        m_terrainTintStrength = visuals.value("tintStrength", m_terrainTintStrength);
        m_terrainDistanceDesatStart = visuals.value("distanceDesatStart", m_terrainDistanceDesatStart);
        m_terrainDistanceDesatEnd = visuals.value("distanceDesatEnd", m_terrainDistanceDesatEnd);
        m_terrainDistanceDesatStrength = visuals.value("distanceDesatStrength", m_terrainDistanceDesatStrength);
        m_terrainDistanceContrastLoss = visuals.value("distanceContrastLoss", m_terrainDistanceContrastLoss);
        m_terrainFogSunScale = visuals.value("fogSunScale", m_terrainFogSunScale);
        if (visuals.contains("tint") && visuals["tint"].is_array() && visuals["tint"].size() == 3) {
            m_terrainTint = Vec3(
                visuals["tint"][0].get<float>(),
                visuals["tint"][1].get<float>(),
                visuals["tint"][2].get<float>()
            );
        }
    }
    if (m_terrainHeightMax <= m_terrainHeightMin) {
        m_terrainHeightMax = m_terrainHeightMin + 1.0f;
    }
    m_terrainFogDistance = std::max(1.0f, m_terrainFogDistance);
    m_terrainDistanceDesatStart = std::max(0.0f, m_terrainDistanceDesatStart);
    m_terrainDistanceDesatEnd = std::max(m_terrainDistanceDesatStart + 1.0f, m_terrainDistanceDesatEnd);
    m_terrainDistanceDesatStrength = std::clamp(m_terrainDistanceDesatStrength, 0.0f, 1.0f);
    m_terrainDistanceContrastLoss = std::clamp(m_terrainDistanceContrastLoss, 0.0f, 1.0f);
    m_terrainFogSunScale = std::clamp(m_terrainFogSunScale, 0.0f, 0.95f);

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

    m_compiledTileSizeMeters = std::max(1.0f, m_compiledTileSizeMeters);
    m_compiledGridResolution = std::max(2, m_compiledGridResolution);
    m_compiledVisibleRadius = std::max(0, m_compiledVisibleRadius);
    m_compiledLoadsPerFrame = std::max(1, m_compiledLoadsPerFrame);

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

    if (config.contains("terrainVisuals") && config["terrainVisuals"].is_object()) {
        const auto& visuals = config["terrainVisuals"];
        m_terrainHeightMin = visuals.value("heightMin", m_terrainHeightMin);
        m_terrainHeightMax = visuals.value("heightMax", m_terrainHeightMax);
        m_terrainNoiseScale = visuals.value("noiseScale", m_terrainNoiseScale);
        m_terrainNoiseStrength = visuals.value("noiseStrength", m_terrainNoiseStrength);
        m_terrainSlopeStart = visuals.value("slopeStart", m_terrainSlopeStart);
        m_terrainSlopeEnd = visuals.value("slopeEnd", m_terrainSlopeEnd);
        m_terrainSlopeDarken = visuals.value("slopeDarken", m_terrainSlopeDarken);
        m_terrainFogDistance = visuals.value("fogDistance", m_terrainFogDistance);
        m_terrainDesaturate = visuals.value("desaturate", m_terrainDesaturate);
        m_terrainTintStrength = visuals.value("tintStrength", m_terrainTintStrength);
        m_terrainDistanceDesatStart = visuals.value("distanceDesatStart", m_terrainDistanceDesatStart);
        m_terrainDistanceDesatEnd = visuals.value("distanceDesatEnd", m_terrainDistanceDesatEnd);
        m_terrainDistanceDesatStrength = visuals.value("distanceDesatStrength", m_terrainDistanceDesatStrength);
        m_terrainDistanceContrastLoss = visuals.value("distanceContrastLoss", m_terrainDistanceContrastLoss);
        m_terrainFogSunScale = visuals.value("fogSunScale", m_terrainFogSunScale);
        if (visuals.contains("tint") && visuals["tint"].is_array() && visuals["tint"].size() == 3) {
            m_terrainTint = Vec3(
                visuals["tint"][0].get<float>(),
                visuals["tint"][1].get<float>(),
                visuals["tint"][2].get<float>()
            );
        }
    }
    if (m_terrainHeightMax <= m_terrainHeightMin) {
        m_terrainHeightMax = m_terrainHeightMin + 1.0f;
    }
    m_terrainFogDistance = std::max(1.0f, m_terrainFogDistance);
    m_terrainDistanceDesatStart = std::max(0.0f, m_terrainDistanceDesatStart);
    m_terrainDistanceDesatEnd = std::max(m_terrainDistanceDesatStart + 1.0f, m_terrainDistanceDesatEnd);
    m_terrainDistanceDesatStrength = std::clamp(m_terrainDistanceDesatStrength, 0.0f, 1.0f);
    m_terrainDistanceContrastLoss = std::clamp(m_terrainDistanceContrastLoss, 0.0f, 1.0f);
    m_terrainFogSunScale = std::clamp(m_terrainFogSunScale, 0.0f, 0.95f);

    m_textured = false;
    m_compiled = true;
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

float smoothstepLocal(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Vec3 mixColor(const Vec3& a, const Vec3& b, float t) {
    return a * (1.0f - t) + b * t;
}

Vec3 terrainFogColor(const Vec3& sunDir) {
    Vec3 dir = sunDir.normalized();
    float elevation = std::clamp(dir.y, -1.0f, 1.0f);
    float dayFactor = smoothstepLocal(-0.25f, 0.35f, elevation);
    float duskFactor = smoothstepLocal(-0.35f, 0.12f, elevation)
        * (1.0f - smoothstepLocal(0.15f, 0.5f, elevation));

    Vec3 horizonDay(0.72f, 0.82f, 0.94f);
    Vec3 horizonNight(0.04f, 0.06f, 0.12f);
    Vec3 horizonDusk(0.96f, 0.55f, 0.32f);

    Vec3 horizon = mixColor(horizonNight, horizonDay, dayFactor);
    horizon = mixColor(horizon, horizonDusk, duskFactor);
    return horizon;
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

bool TerrainRenderer::loadCompiledMesh(const std::string& path, std::vector<float>& out) const {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    char magic[4] = {};
    in.read(magic, 4);
    if (in.gcount() != 4 || std::string(magic, 4) != "NTM1") {
        return false;
    }
    std::uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in || count == 0) {
        return false;
    }
    out.resize(count);
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(count * sizeof(float)));
    return static_cast<std::size_t>(in.gcount()) == count * sizeof(float);
}

bool TerrainRenderer::loadCompiledMask(const std::string& path, int expectedRes, std::vector<std::uint8_t>& out) const {
    if (expectedRes <= 0) {
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    size_t size = static_cast<size_t>(expectedRes * expectedRes);
    out.resize(size);
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(in.gcount()) == size;
}

Vec3 TerrainRenderer::maskClassColor(std::uint8_t cls) const {
    switch (cls) {
        case 1: return Vec3(0.14f, 0.32f, 0.55f);
        case 2: return Vec3(0.56f, 0.54f, 0.5f);
        case 3: return Vec3(0.2f, 0.42f, 0.22f);
        case 4: return Vec3(0.46f, 0.55f, 0.32f);
        default: return Vec3(1.0f, 1.0f, 1.0f);
    }
}

void TerrainRenderer::applyMaskToVerts(std::vector<float>& verts, const std::vector<std::uint8_t>& mask,
                                       int maskRes, float tileMinX, float tileMinZ) const {
    if (maskRes <= 0 || mask.empty()) {
        return;
    }
    float tileSize = m_compiledTileSizeMeters;
    int stride = 9;
    size_t vertexCount = verts.size() / stride;
    for (size_t i = 0; i < vertexCount; ++i) {
        float px = verts[i * stride + 0];
        float pz = verts[i * stride + 2];
        float fx = (px - tileMinX) / tileSize;
        float fz = (pz - tileMinZ) / tileSize;
        float mx = fx * (maskRes - 1);
        float mz = fz * (maskRes - 1);
        int x0 = std::clamp(static_cast<int>(std::floor(mx)), 0, maskRes - 1);
        int z0 = std::clamp(static_cast<int>(std::floor(mz)), 0, maskRes - 1);
        int x1 = std::min(x0 + 1, maskRes - 1);
        int z1 = std::min(z0 + 1, maskRes - 1);
        float tx = mx - static_cast<float>(x0);
        float tz = mz - static_cast<float>(z0);

        auto clsAt = [&](int x, int z) {
            return mask[static_cast<size_t>(z) * maskRes + static_cast<size_t>(x)];
        };

        float w00 = (1.0f - tx) * (1.0f - tz);
        float w10 = tx * (1.0f - tz);
        float w01 = (1.0f - tx) * tz;
        float w11 = tx * tz;

        float total = 0.0f;
        Vec3 blended(0.0f, 0.0f, 0.0f);

        std::uint8_t c00 = clsAt(x0, z0);
        if (c00 != 0) { blended = blended + maskClassColor(c00) * w00; total += w00; }
        std::uint8_t c10 = clsAt(x1, z0);
        if (c10 != 0) { blended = blended + maskClassColor(c10) * w10; total += w10; }
        std::uint8_t c01 = clsAt(x0, z1);
        if (c01 != 0) { blended = blended + maskClassColor(c01) * w01; total += w01; }
        std::uint8_t c11 = clsAt(x1, z1);
        if (c11 != 0) { blended = blended + maskClassColor(c11) * w11; total += w11; }

        if (total <= 0.0f) {
            continue;
        }

        Vec3 base(verts[i * stride + 6], verts[i * stride + 7], verts[i * stride + 8]);
        Vec3 maskColor = blended * (1.0f / total);
        Vec3 mixed = base * (1.0f - total) + maskColor * total;
        verts[i * stride + 6] = mixed.x;
        verts[i * stride + 7] = mixed.y;
        verts[i * stride + 8] = mixed.z;
    }
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
    if (!loadCompiledMesh(meshPath.string(), verts)) {
        if (m_compiledDebugLog) {
            std::cout << "[terrain] missing compiled tile " << x << "," << y << "\n";
        }
        return nullptr;
    }

    if (m_compiledMaskResolution > 0) {
        std::filesystem::path maskPath = std::filesystem::path(m_compiledManifestDir)
            / "tiles" / ("tile_" + std::to_string(x) + "_" + std::to_string(y) + ".mask");
        std::vector<std::uint8_t> mask;
        if (loadCompiledMask(maskPath.string(), m_compiledMaskResolution, mask)) {
            float tileMinX = static_cast<float>(x) * m_compiledTileSizeMeters;
            float tileMinZ = static_cast<float>(y) * m_compiledTileSizeMeters;
            applyMaskToVerts(verts, mask, m_compiledMaskResolution, tileMinX, tileMinZ);
        }
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->init(verts);
    m_compiledTilesLoadedThisFrame += 1;

    TileResource resource;
    resource.ownedMesh = std::move(mesh);
    resource.mesh = resource.ownedMesh.get();
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
            m_shader->setBool("uTerrainShading", true);
            m_shader->setVec3("uCameraPos", cameraPos);
            m_shader->setVec3("uTerrainFogColor", terrainFogColor(sunDir));
            float elevation = std::clamp(sunDir.normalized().y, -1.0f, 1.0f);
            float dayFactor = smoothstepLocal(-0.25f, 0.35f, elevation);
            float fogScale = 1.0f - m_terrainFogSunScale * (1.0f - dayFactor);
            float fogDistance = m_terrainFogDistance * std::max(0.15f, fogScale);
            m_shader->setFloat("uTerrainHeightMin", m_terrainHeightMin);
            m_shader->setFloat("uTerrainHeightMax", m_terrainHeightMax);
            m_shader->setFloat("uTerrainNoiseScale", m_terrainNoiseScale);
            m_shader->setFloat("uTerrainNoiseStrength", m_terrainNoiseStrength);
            m_shader->setFloat("uTerrainSlopeStart", m_terrainSlopeStart);
            m_shader->setFloat("uTerrainSlopeEnd", m_terrainSlopeEnd);
            m_shader->setFloat("uTerrainSlopeDarken", m_terrainSlopeDarken);
            m_shader->setFloat("uTerrainFogDistance", fogDistance);
            m_shader->setFloat("uTerrainDesaturate", m_terrainDesaturate);
            m_shader->setVec3("uTerrainTint", m_terrainTint);
            m_shader->setFloat("uTerrainTintStrength", m_terrainTintStrength);
            m_shader->setFloat("uTerrainDistanceDesatStart", m_terrainDistanceDesatStart);
            m_shader->setFloat("uTerrainDistanceDesatEnd", m_terrainDistanceDesatEnd);
            m_shader->setFloat("uTerrainDistanceDesatStrength", m_terrainDistanceDesatStrength);
            m_shader->setFloat("uTerrainDistanceContrastLoss", m_terrainDistanceContrastLoss);
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
            m_shader->setBool("uTerrainShading", true);
            m_shader->setVec3("uCameraPos", cameraPos);
            m_shader->setVec3("uTerrainFogColor", terrainFogColor(sunDir));
            float elevation = std::clamp(sunDir.normalized().y, -1.0f, 1.0f);
            float dayFactor = smoothstepLocal(-0.25f, 0.35f, elevation);
            float fogScale = 1.0f - m_terrainFogSunScale * (1.0f - dayFactor);
            float fogDistance = m_terrainFogDistance * std::max(0.15f, fogScale);
            m_shader->setFloat("uTerrainHeightMin", m_terrainHeightMin);
            m_shader->setFloat("uTerrainHeightMax", m_terrainHeightMax);
            m_shader->setFloat("uTerrainNoiseScale", m_terrainNoiseScale);
            m_shader->setFloat("uTerrainNoiseStrength", m_terrainNoiseStrength);
            m_shader->setFloat("uTerrainSlopeStart", m_terrainSlopeStart);
            m_shader->setFloat("uTerrainSlopeEnd", m_terrainSlopeEnd);
            m_shader->setFloat("uTerrainSlopeDarken", m_terrainSlopeDarken);
            m_shader->setFloat("uTerrainFogDistance", fogDistance);
            m_shader->setFloat("uTerrainDesaturate", m_terrainDesaturate);
            m_shader->setVec3("uTerrainTint", m_terrainTint);
            m_shader->setFloat("uTerrainTintStrength", m_terrainTintStrength);
            m_shader->setFloat("uTerrainDistanceDesatStart", m_terrainDistanceDesatStart);
            m_shader->setFloat("uTerrainDistanceDesatEnd", m_terrainDistanceDesatEnd);
            m_shader->setFloat("uTerrainDistanceDesatStrength", m_terrainDistanceDesatStrength);
            m_shader->setFloat("uTerrainDistanceContrastLoss", m_terrainDistanceContrastLoss);
            tile->mesh->draw();
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
