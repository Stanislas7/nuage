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
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>

namespace nuage {

namespace {
constexpr int kMaxVisibleRadius = 8;
constexpr int kMaxLoadsPerFrame = 8;
constexpr float kMetersPerKm = 1000.0f;
constexpr float kSqMetersPerSqKm = 1000000.0f;
constexpr double kFtToM = 0.3048;

float rand01(std::uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>((state >> 8) & 0x00FFFFFFu) / 16777215.0f;
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

std::uint32_t hashTileSeed(int x, int y, int seed) {
    std::uint32_t h = 2166136261u;
    auto mix = [&](std::uint32_t v) {
        h ^= v;
        h *= 16777619u;
    };
    mix(static_cast<std::uint32_t>(x));
    mix(static_cast<std::uint32_t>(y));
    mix(static_cast<std::uint32_t>(seed));
    return h;
}

bool sampleGrid(const std::vector<float>& gridVerts, int res, float tileMinX, float tileMinZ, float tileSize,
                float worldX, float worldZ, float& outHeight, Vec3& outNormal,
                float& outWater, float& outUrban, float& outForest) {
    if (res < 2 || tileSize <= 0.0f) {
        return false;
    }
    float fx = (worldX - tileMinX) / tileSize;
    float fz = (worldZ - tileMinZ) / tileSize;
    fx = std::clamp(fx, 0.0f, 1.0f);
    fz = std::clamp(fz, 0.0f, 1.0f);
    float gx = fx * (res - 1);
    float gz = fz * (res - 1);
    int x0 = static_cast<int>(std::floor(gx));
    int z0 = static_cast<int>(std::floor(gz));
    int x1 = std::min(x0 + 1, res - 1);
    int z1 = std::min(z0 + 1, res - 1);
    float tx = gx - static_cast<float>(x0);
    float tz = gz - static_cast<float>(z0);

    auto sample = [&](int x, int z, int offset) -> float {
        size_t idx = static_cast<size_t>(z * res + x) * 9 + offset;
        return gridVerts[idx];
    };

    float h00 = sample(x0, z0, 1);
    float h10 = sample(x1, z0, 1);
    float h01 = sample(x0, z1, 1);
    float h11 = sample(x1, z1, 1);
    float h0 = lerp(h00, h10, tx);
    float h1 = lerp(h01, h11, tx);
    outHeight = lerp(h0, h1, tz);

    Vec3 n00(sample(x0, z0, 3), sample(x0, z0, 4), sample(x0, z0, 5));
    Vec3 n10(sample(x1, z0, 3), sample(x1, z0, 4), sample(x1, z0, 5));
    Vec3 n01(sample(x0, z1, 3), sample(x0, z1, 4), sample(x0, z1, 5));
    Vec3 n11(sample(x1, z1, 3), sample(x1, z1, 4), sample(x1, z1, 5));
    Vec3 n0 = n00 * (1.0f - tx) + n10 * tx;
    Vec3 n1 = n01 * (1.0f - tx) + n11 * tx;
    outNormal = (n0 * (1.0f - tz) + n1 * tz).normalized();

    float w00 = sample(x0, z0, 6);
    float w10 = sample(x1, z0, 6);
    float w01 = sample(x0, z1, 6);
    float w11 = sample(x1, z1, 6);
    float w0 = lerp(w00, w10, tx);
    float w1 = lerp(w01, w11, tx);
    outWater = lerp(w0, w1, tz);

    float u00 = sample(x0, z0, 7);
    float u10 = sample(x1, z0, 7);
    float u01 = sample(x0, z1, 7);
    float u11 = sample(x1, z1, 7);
    float u0 = lerp(u00, u10, tx);
    float u1 = lerp(u01, u11, tx);
    outUrban = lerp(u0, u1, tz);

    float f00 = sample(x0, z0, 8);
    float f10 = sample(x1, z0, 8);
    float f01 = sample(x0, z1, 8);
    float f11 = sample(x1, z1, 8);
    float f0 = lerp(f00, f10, tx);
    float f1 = lerp(f01, f11, tx);
    outForest = lerp(f0, f1, tz);

    return true;
}

void appendTriangle(std::vector<float>& verts, const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& color) {
    Vec3 normal = (b - a).cross(c - a).normalized();
    auto pushVert = [&](const Vec3& p) {
        verts.insert(verts.end(), {p.x, p.y, p.z, normal.x, normal.y, normal.z, color.x, color.y, color.z});
    };
    pushVert(a);
    pushVert(b);
    pushVert(c);
}

std::unique_ptr<Mesh> buildTreeMeshForTile(const std::vector<float>& gridVerts, int res, int tileX, int tileY,
                                           float tileMinX, float tileMinZ, float tileSize, bool useWaterMask,
                                           bool enabled, float densityPerSqKm, float minHeight,
                                           float maxHeight, float minRadius, float maxRadius,
                                           float maxSlope, int seed) {
    if (!enabled || densityPerSqKm <= 0.0f || res < 2) {
        return nullptr;
    }
    float areaSqKm = (tileSize * tileSize) / kSqMetersPerSqKm;
    int targetCount = static_cast<int>(std::round(areaSqKm * densityPerSqKm));
    if (targetCount <= 0) {
        return nullptr;
    }

    std::vector<float> verts;
    verts.reserve(static_cast<size_t>(targetCount) * 6 * 18);

    std::uint32_t rng = hashTileSeed(tileX, tileY, seed);

    int placed = 0;
    int attempts = targetCount * 4 + 12;
    float margin = tileSize * 0.02f;
    int sides = 6;

    while (placed < targetCount && attempts-- > 0) {
        float rx = rand01(rng);
        float rz = rand01(rng);
        float x = tileMinX + margin + rx * (tileSize - 2.0f * margin);
        float z = tileMinZ + margin + rz * (tileSize - 2.0f * margin);

        float height = 0.0f;
        Vec3 normal;
        float water = 0.0f;
        float urban = 0.0f;
        float forest = 0.0f;
        if (!sampleGrid(gridVerts, res, tileMinX, tileMinZ, tileSize, x, z,
                        height, normal, water, urban, forest)) {
            continue;
        }
        float slope = 1.0f - std::clamp(normal.y, 0.0f, 1.0f);
        if (slope > maxSlope) {
            continue;
        }
        if (useWaterMask && water > 0.35f) {
            continue;
        }
        if (urban > 0.35f) {
            continue;
        }
        if (useWaterMask) {
            float forestChance = std::clamp(forest, 0.0f, 1.0f);
            if (rand01(rng) > forestChance) {
                continue;
            }
        }

        float treeHeight = lerp(minHeight, maxHeight, rand01(rng));
        float canopyRadius = lerp(minRadius, maxRadius, rand01(rng));
        float trunkHeight = treeHeight * 0.32f;
        float trunkRadius = canopyRadius * 0.2f;

        Vec3 trunkColor(0.36f + rand01(rng) * 0.05f, 0.24f + rand01(rng) * 0.04f, 0.14f);
        Vec3 canopyColor(0.07f, 0.32f + rand01(rng) * 0.12f, 0.12f + rand01(rng) * 0.05f);

        Vec3 base(x, height, z);
        for (int i = 0; i < sides; ++i) {
            float a0 = (static_cast<float>(i) / sides) * 6.2831853f;
            float a1 = (static_cast<float>(i + 1) / sides) * 6.2831853f;
            Vec3 p0 = base + Vec3(std::cos(a0) * trunkRadius, 0.0f, std::sin(a0) * trunkRadius);
            Vec3 p1 = base + Vec3(std::cos(a1) * trunkRadius, 0.0f, std::sin(a1) * trunkRadius);
            Vec3 p2 = base + Vec3(std::cos(a1) * trunkRadius, trunkHeight, std::sin(a1) * trunkRadius);
            Vec3 p3 = base + Vec3(std::cos(a0) * trunkRadius, trunkHeight, std::sin(a0) * trunkRadius);
            appendTriangle(verts, p0, p1, p2, trunkColor);
            appendTriangle(verts, p0, p2, p3, trunkColor);
        }

        Vec3 canopyBase = base + Vec3(0.0f, trunkHeight, 0.0f);
        Vec3 apex = canopyBase + Vec3(0.0f, treeHeight - trunkHeight, 0.0f);
        for (int i = 0; i < sides; ++i) {
            float a0 = (static_cast<float>(i) / sides) * 6.2831853f;
            float a1 = (static_cast<float>(i + 1) / sides) * 6.2831853f;
            Vec3 b0 = canopyBase + Vec3(std::cos(a0) * canopyRadius, 0.0f, std::sin(a0) * canopyRadius);
            Vec3 b1 = canopyBase + Vec3(std::cos(a1) * canopyRadius, 0.0f, std::sin(a1) * canopyRadius);
            appendTriangle(verts, b0, b1, apex, canopyColor);
        }

        placed += 1;
    }

    if (verts.empty()) {
        return nullptr;
    }
    auto mesh = std::make_unique<Mesh>();
    mesh->init(verts);
    return mesh;
}

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

bool parseNumber(const nlohmann::json& value, double& out) {
    if (value.is_number_float() || value.is_number_integer() || value.is_number_unsigned()) {
        out = value.get<double>();
        return true;
    }
    if (value.is_string()) {
        const std::string s = value.get<std::string>();
        if (s.empty()) return false;
        char* end = nullptr;
        out = std::strtod(s.c_str(), &end);
        return end != s.c_str();
    }
    return false;
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

void TerrainRenderer::setTreesEnabled(bool enabled) {
    if (m_treesEnabled == enabled) {
        return;
    }
    m_treesEnabled = enabled;
    if (!m_tileCache.empty()) {
        m_tileCache.clear();
        m_procTileCreateCounts.clear();
        m_compiledTileCreateCounts.clear();
        m_procTileRebuilds = 0;
        m_compiledTileRebuilds = 0;
    }
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
    m_treesEnabled = false;
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
    m_compiledOriginValid = false;
    if (manifest.contains("originLLA") && manifest["originLLA"].is_array() && manifest["originLLA"].size() == 3) {
        m_compiledOrigin.latDeg = manifest["originLLA"][0].get<double>();
        m_compiledOrigin.lonDeg = manifest["originLLA"][1].get<double>();
        m_compiledOrigin.altMeters = manifest["originLLA"][2].get<double>();
        m_compiledOriginValid = true;
    }

    m_compiledVisibleRadius = config.value("compiledVisibleRadius", 1);
    m_compiledLoadsPerFrame = config.value("compiledMaxLoadsPerFrame", 2);
    m_compiledDebugLog = config.value("compiledDebugLog", true);
    m_compiledLod1Distance = config.value("compiledLod1Distance", m_compiledTileSizeMeters * 1.5f);
    m_compiledSkirtDepth = config.value("compiledSkirtDepth", m_compiledTileSizeMeters * 0.05f);
    if (config.contains("terrainTrees") && config["terrainTrees"].is_object()) {
        const auto& trees = config["terrainTrees"];
        m_treesEnabled = trees.value("enabled", m_treesEnabled);
        m_treesDensityPerSqKm = trees.value("densityPerSqKm", m_treesDensityPerSqKm);
        m_treesMinHeight = trees.value("minHeight", m_treesMinHeight);
        m_treesMaxHeight = trees.value("maxHeight", m_treesMaxHeight);
        m_treesMinRadius = trees.value("minRadius", m_treesMinRadius);
        m_treesMaxRadius = trees.value("maxRadius", m_treesMaxRadius);
        m_treesMaxSlope = trees.value("maxSlope", m_treesMaxSlope);
        m_treesMaxDistance = trees.value("maxDistance", m_treesMaxDistance);
        m_treesSeed = trees.value("seed", m_treesSeed);
    }

    m_compiledTileSizeMeters = std::max(1.0f, m_compiledTileSizeMeters);
    m_compiledGridResolution = std::max(2, m_compiledGridResolution);
    m_compiledVisibleRadius = std::max(0, m_compiledVisibleRadius);
    m_compiledLoadsPerFrame = std::max(1, m_compiledLoadsPerFrame);
    m_compiledLod1Distance = std::max(0.0f, m_compiledLod1Distance);
    m_compiledLod1DistanceSq = m_compiledLod1Distance * m_compiledLod1Distance;
    m_compiledSkirtDepth = std::max(0.0f, m_compiledSkirtDepth);
    m_treesDensityPerSqKm = std::max(0.0f, m_treesDensityPerSqKm);
    m_treesMinHeight = std::max(0.1f, m_treesMinHeight);
    m_treesMaxHeight = std::max(m_treesMinHeight, m_treesMaxHeight);
    m_treesMinRadius = std::max(0.05f, m_treesMinRadius);
    m_treesMaxRadius = std::max(m_treesMinRadius, m_treesMaxRadius);
    m_treesMaxSlope = std::clamp(m_treesMaxSlope, 0.0f, 1.0f);
    m_treesMaxDistance = std::max(0.0f, m_treesMaxDistance);
    m_treesMaxDistanceSq = m_treesMaxDistance * m_treesMaxDistance;

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
    loadRunways(config, configPath);

    m_textured = false;
    m_compiled = true;
}

Vec3 TerrainRenderer::compiledGeoToWorld(double latDeg, double lonDeg, double altMeters) const {
    if (!m_compiledOriginValid) {
        return Vec3(0.0f, 0.0f, 0.0f);
    }
    return llaToEnu(m_compiledOrigin, latDeg, lonDeg, altMeters);
}

bool TerrainRenderer::sampleSurface(float worldX, float worldZ, TerrainSample& outSample) const {
    outSample = TerrainSample{};

    if (sampleRunway(worldX, worldZ, outSample)) {
        return true;
    }

    if (m_procedural) {
        outSample.height = proceduralHeight(worldX, worldZ);
        outSample.normal = proceduralNormal(worldX, worldZ);
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

    if (m_procedural) {
        outSample.height = proceduralHeight(worldX, worldZ);
        outSample.normal = proceduralNormal(worldX, worldZ);
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

bool TerrainRenderer::sampleRunway(float worldX, float worldZ, TerrainSample& outSample) const {
    if (!m_runwaysEnabled || m_runwayColliders.empty()) {
        return false;
    }
    for (const auto& runway : m_runwayColliders) {
        Vec3 delta(worldX - runway.center.x, 0.0f, worldZ - runway.center.z);
        float along = delta.x * runway.dir.x + delta.z * runway.dir.z;
        float side = delta.x * runway.perp.x + delta.z * runway.perp.z;
        if (std::abs(along) > runway.halfLength || std::abs(side) > runway.halfWidth) {
            continue;
        }
        float t = (along + runway.halfLength) / (2.0f * runway.halfLength);
        t = std::clamp(t, 0.0f, 1.0f);
        float runwayY = runway.h0 + (runway.h1 - runway.h0) * t;

        float slopeY = (runway.h1 - runway.h0) / std::max(0.001f, runway.halfLength * 2.0f);
        Vec3 dirSlope(runway.dir.x, slopeY, runway.dir.z);
        Vec3 perpFlat(runway.perp.x, 0.0f, runway.perp.z);
        Vec3 normal = perpFlat.cross(dirSlope);
        if (normal.length() < 1e-4f) {
            normal = Vec3(0.0f, 1.0f, 0.0f);
        } else {
            normal = normal.normalized();
        }

        outSample.height = runwayY + m_runwayHeightOffset;
        outSample.normal = normal;
        outSample.water = 0.0f;
        outSample.urban = 0.0f;
        outSample.forest = 0.0f;
        outSample.onRunway = true;
        return true;
    }
    return false;
}

bool TerrainRenderer::sampleCompiledSurface(int tx, int ty, float worldX, float worldZ,
                                            bool forceLoad, TerrainSample& outSample) const {
    if (m_compiledTiles.find(packedTileKey(tx, ty)) == m_compiledTiles.end()) {
        return false;
    }
    auto* tile = const_cast<TerrainRenderer*>(this)->ensureCompiledTileLoaded(tx, ty, forceLoad);
    if (!tile || !tile->hasGrid || tile->gridVerts.empty() || tile->gridRes <= 1) {
        return false;
    }

    float tileMinX = static_cast<float>(tx) * m_compiledTileSizeMeters;
    float tileMinZ = static_cast<float>(ty) * m_compiledTileSizeMeters;
    return sampleGrid(tile->gridVerts, tile->gridRes, tileMinX, tileMinZ, m_compiledTileSizeMeters,
                      worldX, worldZ, outSample.height, outSample.normal,
                      outSample.water, outSample.urban, outSample.forest);
}

bool TerrainRenderer::sampleCompiledSurfaceCached(int tx, int ty, float worldX, float worldZ,
                                                  TerrainSample& outSample) const {
    if (m_compiledTiles.find(packedTileKey(tx, ty)) == m_compiledTiles.end()) {
        return false;
    }
    std::string key = "C_x" + std::to_string(tx) + "_y" + std::to_string(ty);
    auto it = m_tileCache.find(key);
    if (it == m_tileCache.end()) {
        return false;
    }
    const TileResource& tile = it->second;
    if (!tile.hasGrid || tile.gridVerts.empty() || tile.gridRes <= 1) {
        return false;
    }

    float tileMinX = static_cast<float>(tx) * m_compiledTileSizeMeters;
    float tileMinZ = static_cast<float>(ty) * m_compiledTileSizeMeters;
    return sampleGrid(tile.gridVerts, tile.gridRes, tileMinX, tileMinZ, m_compiledTileSizeMeters,
                      worldX, worldZ, outSample.height, outSample.normal,
                      outSample.water, outSample.urban, outSample.forest);
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
    m_textureSettings.macroScale = texConfig.value("macroScale", m_textureSettings.macroScale);
    m_textureSettings.macroStrength = texConfig.value("macroStrength", m_textureSettings.macroStrength);
    m_textureSettings.farmlandStrength = texConfig.value("farmlandStrength", m_textureSettings.farmlandStrength);
    m_textureSettings.farmlandStripeScale = texConfig.value("farmlandStripeScale", m_textureSettings.farmlandStripeScale);
    m_textureSettings.farmlandStripeContrast = texConfig.value("farmlandStripeContrast", m_textureSettings.farmlandStripeContrast);
    m_textureSettings.scrubStrength = texConfig.value("scrubStrength", m_textureSettings.scrubStrength);
    m_textureSettings.scrubNoiseScale = texConfig.value("scrubNoiseScale", m_textureSettings.scrubNoiseScale);
    m_textureSettings.grassTintStrength = texConfig.value("grassTintStrength", m_textureSettings.grassTintStrength);
    m_textureSettings.forestTintStrength = texConfig.value("forestTintStrength", m_textureSettings.forestTintStrength);
    m_textureSettings.urbanTintStrength = texConfig.value("urbanTintStrength", m_textureSettings.urbanTintStrength);
    m_textureSettings.microScale = texConfig.value("microScale", m_textureSettings.microScale);
    m_textureSettings.microStrength = texConfig.value("microStrength", m_textureSettings.microStrength);
    m_textureSettings.waterDetailScale = texConfig.value("waterDetailScale", m_textureSettings.waterDetailScale);
    m_textureSettings.waterDetailStrength = texConfig.value("waterDetailStrength", m_textureSettings.waterDetailStrength);
    auto loadTint = [&](const char* key, Vec3& out) {
        if (texConfig.contains(key) && texConfig[key].is_array() && texConfig[key].size() == 3) {
            out = Vec3(texConfig[key][0].get<float>(),
                       texConfig[key][1].get<float>(),
                       texConfig[key][2].get<float>());
        }
    };
    loadTint("grassTintA", m_textureSettings.grassTintA);
    loadTint("grassTintB", m_textureSettings.grassTintB);
    loadTint("forestTintA", m_textureSettings.forestTintA);
    loadTint("forestTintB", m_textureSettings.forestTintB);
    loadTint("urbanTintA", m_textureSettings.urbanTintA);
    loadTint("urbanTintB", m_textureSettings.urbanTintB);
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
    loadTex("grassNormal", m_texGrassNormal, "terrain_grass_n");
    loadTex("dirtNormal", m_texDirtNormal, "terrain_dirt_n");
    loadTex("rockNormal", m_texRockNormal, "terrain_rock_n");
    loadTex("urbanNormal", m_texUrbanNormal, "terrain_urban_n");
    loadTex("grassRoughness", m_texGrassRough, "terrain_grass_r");
    loadTex("dirtRoughness", m_texDirtRough, "terrain_dirt_r");
    loadTex("rockRoughness", m_texRockRough, "terrain_rock_r");
    loadTex("urbanRoughness", m_texUrbanRough, "terrain_urban_r");
    if (!loadedAny) {
        m_textureSettings.enabled = false;
    }
}

void TerrainRenderer::loadRunways(const nlohmann::json& config, const std::string& configPath) {
    m_runwayMesh.reset();
    m_runwaysEnabled = false;
    m_runwayColliders.clear();
    m_runwayTexture = nullptr;

    if (!config.contains("runways") || !config["runways"].is_object()) {
        return;
    }
    const auto& runwaysConfig = config["runways"];
    m_runwaysEnabled = runwaysConfig.value("enabled", true);
    if (!m_runwaysEnabled) {
        return;
    }
    float runwayTexScaleU = runwaysConfig.value("textureScaleU", 5.0f);
    float runwayTexScaleV = runwaysConfig.value("textureScaleV", 30.0f);

    std::filesystem::path cfg(configPath);
    auto resolve = [&](const std::string& p) {
        if (p.empty()) return p;
        std::filesystem::path path(p);
        if (path.is_absolute()) return p;
        return (cfg.parent_path() / path).string();
    };

    std::string runwayTexPath = resolve(runwaysConfig.value("texture", ""));
    if (!runwayTexPath.empty() && m_assets) {
        if (m_assets->loadTexture("runway_tex", runwayTexPath, true)) {
            m_runwayTexture = m_assets->getTexture("runway_tex");
        }
    }

    std::string runwaysPath = resolve(runwaysConfig.value("json", ""));
    if (runwaysPath.empty()) {
        return;
    }

    auto runwaysOpt = loadJsonConfig(runwaysPath);
    if (!runwaysOpt) {
        std::cerr << "Failed to load runways JSON: " << runwaysPath << "\n";
        return;
    }
    const auto& runways = *runwaysOpt;

    if (runwaysConfig.contains("color") && runwaysConfig["color"].is_array()
        && runwaysConfig["color"].size() == 3) {
        m_runwayColor = Vec3(runwaysConfig["color"][0].get<float>(),
                             runwaysConfig["color"][1].get<float>(),
                             runwaysConfig["color"][2].get<float>());
    }
    m_runwayHeightOffset = std::max(0.0f, runwaysConfig.value("heightOffset", m_runwayHeightOffset));

    std::vector<float> verts;
    constexpr int stride = 8; // pos(3) normal(3) uv(2)
    if (runways.contains("runways") && runways["runways"].is_array()) {
        for (const auto& runway : runways["runways"]) {
            if (!runway.is_object()) {
                continue;
            }
            std::string closed = runway.value("closed", "");
            if (closed == "1" || closed == "true" || closed == "TRUE") {
                continue;
            }
            if (!runway.contains("leENU") || !runway.contains("heENU")) {
                continue;
            }
            const auto& le = runway["leENU"];
            const auto& he = runway["heENU"];
            if (!le.is_array() || !he.is_array() || le.size() != 3 || he.size() != 3) {
                continue;
            }

            double widthFt = 0.0;
            if (!runway.contains("widthFt") || !parseNumber(runway["widthFt"], widthFt)) {
                continue;
            }
            float widthMeters = static_cast<float>(widthFt * kFtToM);
            if (widthMeters <= 0.1f) {
                continue;
            }

            Vec3 lePos(le[0].get<float>(), le[1].get<float>(), le[2].get<float>());
            Vec3 hePos(he[0].get<float>(), he[1].get<float>(), he[2].get<float>());

            float dx = hePos.x - lePos.x;
            float dz = hePos.z - lePos.z;
            float length = std::sqrt(dx * dx + dz * dz);
            if (length < 1.0f) {
                continue;
            }
            Vec3 dir(dx / length, 0.0f, dz / length);
            Vec3 perp(-dir.z, 0.0f, dir.x);
            float halfWidth = widthMeters * 0.5f;
            float halfLength = length * 0.5f;

            RunwayCollider collider;
            collider.center = (lePos + hePos) * 0.5f;
            collider.dir = dir;
            collider.perp = perp;
            collider.halfLength = halfLength;
            collider.halfWidth = halfWidth;
            collider.h0 = lePos.y;
            collider.h1 = hePos.y;
            m_runwayColliders.push_back(collider);

            Vec3 leOffset = perp * halfWidth;
            Vec3 heOffset = perp * halfWidth;

            Vec3 p0(lePos.x + leOffset.x, lePos.y + m_runwayHeightOffset, lePos.z + leOffset.z);
            Vec3 p1(lePos.x - leOffset.x, lePos.y + m_runwayHeightOffset, lePos.z - leOffset.z);
            Vec3 p2(hePos.x - heOffset.x, hePos.y + m_runwayHeightOffset, hePos.z - heOffset.z);
            Vec3 p3(hePos.x + heOffset.x, hePos.y + m_runwayHeightOffset, hePos.z + heOffset.z);
            float uvU0 = (widthMeters * 0.5f) / std::max(0.1f, runwayTexScaleU);
            float uvU1 = -uvU0;
            float uvV0 = 0.0f;
            float uvV1 = length / std::max(0.1f, runwayTexScaleV);

            Vec3 normal(0.0f, 1.0f, 0.0f);
            auto push = [&](const Vec3& pos, const Vec2& uv) {
                verts.insert(verts.end(), {
                    pos.x, pos.y, pos.z,
                    normal.x, normal.y, normal.z,
                    uv.x, uv.y
                });
            };

            push(p0, Vec2(uvU0, uvV0));
            push(p1, Vec2(uvU1, uvV0));
            push(p2, Vec2(uvU1, uvV1));

            push(p0, Vec2(uvU0, uvV0));
            push(p2, Vec2(uvU1, uvV1));
            push(p3, Vec2(uvU0, uvV1));
        }
    }

    if (!verts.empty()) {
        m_runwayMesh = std::make_unique<Mesh>();
        m_runwayMesh->initTextured(verts);
        std::cout << "[runways] loaded " << (verts.size() / 8 / 6) << " runway quads from " << runwaysPath << "\n";
    } else {
        std::cout << "[runways] no runway mesh built from " << runwaysPath << "\n";
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
    shader->setFloat("uTerrainMacroScale", m_textureSettings.macroScale);
    shader->setFloat("uTerrainMacroStrength", m_textureSettings.macroStrength);
    shader->setFloat("uTerrainFarmlandStrength", m_textureSettings.farmlandStrength);
    shader->setFloat("uTerrainFarmlandStripeScale", m_textureSettings.farmlandStripeScale);
    shader->setFloat("uTerrainFarmlandStripeContrast", m_textureSettings.farmlandStripeContrast);
    shader->setFloat("uTerrainScrubStrength", m_textureSettings.scrubStrength);
    shader->setFloat("uTerrainScrubNoiseScale", m_textureSettings.scrubNoiseScale);
    shader->setVec3("uTerrainGrassTintA", m_textureSettings.grassTintA);
    shader->setVec3("uTerrainGrassTintB", m_textureSettings.grassTintB);
    shader->setFloat("uTerrainGrassTintStrength", m_textureSettings.grassTintStrength);
    shader->setVec3("uTerrainForestTintA", m_textureSettings.forestTintA);
    shader->setVec3("uTerrainForestTintB", m_textureSettings.forestTintB);
    shader->setFloat("uTerrainForestTintStrength", m_textureSettings.forestTintStrength);
    shader->setVec3("uTerrainUrbanTintA", m_textureSettings.urbanTintA);
    shader->setVec3("uTerrainUrbanTintB", m_textureSettings.urbanTintB);
    shader->setFloat("uTerrainUrbanTintStrength", m_textureSettings.urbanTintStrength);
    shader->setFloat("uTerrainMicroScale", m_textureSettings.microScale);
    shader->setFloat("uTerrainMicroStrength", m_textureSettings.microStrength);
    shader->setFloat("uTerrainWaterDetailScale", m_textureSettings.waterDetailScale);
    shader->setFloat("uTerrainWaterDetailStrength", m_textureSettings.waterDetailStrength);
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
    auto bindOpt = [&](Texture* tex, int unit, const char* name) {
        if (!tex) return;
        tex->bind(unit);
        shader->setInt(name, unit);
    };
    bindOpt(m_texGrassNormal, 6, "uTerrainTexGrassNormal");
    bindOpt(m_texDirtNormal, 7, "uTerrainTexDirtNormal");
    bindOpt(m_texRockNormal, 8, "uTerrainTexRockNormal");
    bindOpt(m_texUrbanNormal, 9, "uTerrainTexUrbanNormal");
    bindOpt(m_texGrassRough, 10, "uTerrainTexGrassRough");
    bindOpt(m_texDirtRough, 11, "uTerrainTexDirtRough");
    bindOpt(m_texRockRough, 12, "uTerrainTexRockRough");
    bindOpt(m_texUrbanRough, 13, "uTerrainTexUrbanRough");
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

Vec3 TerrainRenderer::proceduralNormal(float worldX, float worldZ) const {
    constexpr float delta = 2.0f;
    float dHx = proceduralHeight(worldX + delta, worldZ) - proceduralHeight(worldX - delta, worldZ);
    float dHz = proceduralHeight(worldX, worldZ + delta) - proceduralHeight(worldX, worldZ - delta);
    Vec3 normal(-dHx / (2.0f * delta), 1.0f, -dHz / (2.0f * delta));
    if (normal.length() < 1e-4f) {
        return Vec3(0.0f, 1.0f, 0.0f);
    }
    return normal.normalized();
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


TerrainRenderer::TileResource* TerrainRenderer::ensureProceduralTileLoaded(int x, int y, bool force) {
    std::string key = "P_x" + std::to_string(x) + "_y" + std::to_string(y);
    auto found = m_tileCache.find(key);
    if (found != m_tileCache.end()) {
        return &found->second;
    }
    if (!force && m_procTilesLoadedThisFrame >= m_procLoadsPerFrame) {
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

TerrainRenderer::TileResource* TerrainRenderer::ensureCompiledTileLoaded(int x, int y, bool force) {
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
    if (!force && m_compiledTilesLoadedThisFrame >= m_compiledLoadsPerFrame) {
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
    std::vector<std::uint8_t> maskData;
    if (m_compiledMaskResolution > 0) {
        std::filesystem::path maskPath = std::filesystem::path(m_compiledManifestDir)
            / "tiles" / ("tile_" + std::to_string(x) + "_" + std::to_string(y) + ".mask");
        if (load_compiled_mask(maskPath.string(), m_compiledMaskResolution, maskData)) {
            apply_mask_to_verts(verts, maskData, m_compiledMaskResolution, m_compiledTileSizeMeters, tileMinX, tileMinZ);
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
    resource.ownedTreeMesh = nullptr;
    resource.treeMesh = nullptr;
    resource.texture = nullptr;
    resource.center = Vec3((static_cast<float>(x) + 0.5f) * m_compiledTileSizeMeters,
                           0.0f,
                           (static_cast<float>(y) + 0.5f) * m_compiledTileSizeMeters);
    resource.radius = m_compiledTileSizeMeters * 0.5f;
    resource.tileMinX = tileMinX;
    resource.tileMinZ = tileMinZ;
    resource.level = 0;
    resource.x = x;
    resource.y = y;
    resource.gridRes = builtGrid ? (m_compiledGridResolution + 1) : 0;
    resource.textured = false;
    resource.procedural = false;
    resource.compiled = true;
    resource.hasGrid = builtGrid;
    if (builtGrid) {
        resource.gridVerts = std::move(gridVerts);
    }
    if (!maskData.empty()) {
        auto tex = std::make_unique<Texture>();
        if (tex->loadFromData(maskData.data(), m_compiledMaskResolution, m_compiledMaskResolution, 1, false)) {
            resource.maskTexture = tex.get();
            resource.ownedMaskTexture = std::move(tex);
        }
    }

    if (builtGrid && m_compiledGridResolution >= 2) {
        int res = m_compiledGridResolution + 1;
        std::vector<float> lodVerts;
        std::vector<std::uint32_t> lodIndices;
        buildLodVertices(resource.gridVerts.empty() ? gridVerts : resource.gridVerts, res, res, 2, lodVerts);
        buildLodIndices(res, res, 2, lodIndices);
        addSkirt(lodVerts, lodIndices, (res - 1) / 2 + 1, (res - 1) / 2 + 1, m_compiledSkirtDepth);
        if (!lodVerts.empty() && !lodIndices.empty()) {
            auto lodMesh = std::make_unique<Mesh>();
            lodMesh->initIndexed(lodVerts, lodIndices);
            resource.ownedMeshLod1 = std::move(lodMesh);
            resource.meshLod1 = resource.ownedMeshLod1.get();
        }
    }

    if (builtGrid && m_treesEnabled) {
        int res = m_compiledGridResolution + 1;
        bool useWaterMask = m_compiledMaskResolution > 0;
        resource.ownedTreeMesh = buildTreeMeshForTile(resource.gridVerts.empty() ? gridVerts : resource.gridVerts,
                                                      res, x, y,
                                                      tileMinX, tileMinZ,
                                                      m_compiledTileSizeMeters, useWaterMask,
                                                      m_treesEnabled, m_treesDensityPerSqKm,
                                                      m_treesMinHeight, m_treesMaxHeight,
                                                      m_treesMinRadius, m_treesMaxRadius,
                                                      m_treesMaxSlope, m_treesSeed);
        resource.treeMesh = resource.ownedTreeMesh.get();
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

void TerrainRenderer::preloadPhysicsAt(float worldX, float worldZ, int radius) {
    if (m_procedural) {
        int cx = static_cast<int>(std::floor(worldX / m_procTileSizeMeters));
        int cy = static_cast<int>(std::floor(worldZ / m_procTileSizeMeters));
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                ensureProceduralTileLoaded(cx + dx, cy + dy, true);
            }
        }
        return;
    }
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
            m_shader->setBool("uTerrainHasMaskTex", false);
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
    std::unordered_map<std::int64_t, bool> wantsLod1;
    wantsLod1.reserve(desiredKeys.size());

    struct VisibleTile {
        TileResource* tile;
        std::int64_t key;
        float distSq;
    };
    std::vector<VisibleTile> visibleTiles;
    visibleTiles.reserve(desiredKeys.size());

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

            float distX = tile->center.x - cameraPos.x;
            float distZ = tile->center.z - cameraPos.z;
            float distSq = distX * distX + distZ * distZ;
            std::int64_t tileKey = packedTileKey(tx, ty);
            bool wants = tile->meshLod1 && m_compiledLod1DistanceSq > 0.0f
                && distSq >= m_compiledLod1DistanceSq;
            wantsLod1[tileKey] = wants;
            visibleTiles.push_back({tile, tileKey, distSq});
        }
    }

    for (const auto& entry : visibleTiles) {
        TileResource* tile = entry.tile;
        if (!tile || !tile->mesh) {
            continue;
        }

        bool useLod1 = false;
        if (tile->meshLod1 && m_compiledLod1DistanceSq > 0.0f) {
            auto it = wantsLod1.find(entry.key);
            if (it != wantsLod1.end() && it->second) {
                auto neighborOk = [&](int dx, int dy) {
                    auto nit = wantsLod1.find(packedTileKey(tile->x + dx, tile->y + dy));
                    return (nit != wantsLod1.end() && nit->second);
                };
                if (neighborOk(-1, 0) && neighborOk(1, 0)
                    && neighborOk(0, -1) && neighborOk(0, 1)) {
                    useLod1 = true;
                }
            }
        }

        m_shader->use();
        m_shader->setMat4("uMVP", vp);
        applyDirectionalLighting(m_shader, sunDir);
        m_visuals.bind(m_shader, sunDir, cameraPos);
        bindTerrainTextures(m_shader, m_compiledMaskResolution > 0);
        bool hasMask = (tile->maskTexture != nullptr);
        m_shader->setBool("uTerrainHasMaskTex", hasMask);
        if (hasMask) {
            tile->maskTexture->bind(5);
            m_shader->setInt("uTerrainMaskTex", 5);
            m_shader->setVec2("uTerrainMaskOrigin", Vec2(tile->tileMinX, tile->tileMinZ));
            float invSize = 1.0f / m_compiledTileSizeMeters;
            m_shader->setVec2("uTerrainMaskInvSize", Vec2(invSize, invSize));
        }
        Mesh* meshToDraw = useLod1 ? tile->meshLod1 : tile->mesh;
        meshToDraw->draw();

        if (m_treesEnabled && tile->treeMesh) {
            bool inRange = (m_treesMaxDistanceSq <= 0.0f) || (entry.distSq <= m_treesMaxDistanceSq);
            bool nearLod0 = (m_compiledLod1DistanceSq <= 0.0f) || (entry.distSq < m_compiledLod1DistanceSq);
            if (inRange && nearLod0) {
                m_shader->use();
                m_shader->setMat4("uMVP", vp);
                applyDirectionalLighting(m_shader, sunDir);
                m_shader->setBool("uTerrainShading", false);
                m_shader->setBool("uTerrainUseTextures", false);
                m_shader->setBool("uTerrainUseMasks", false);
                m_shader->setBool("uUseUniformColor", false);
                tile->treeMesh->draw();
            }
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

    if (m_runwaysEnabled && m_runwayMesh) {
        Shader* rs = (m_texturedShader && m_runwayTexture) ? m_texturedShader : m_shader;
        rs->use();
        rs->setMat4("uMVP", vp);
        applyDirectionalLighting(rs, sunDir);
        if (rs == m_texturedShader && m_runwayTexture) {
            if (m_compiledDebugLog) {
                std::cout << "[runways] drawing textured runway mesh\n";
            }
        } else if (m_compiledDebugLog) {
            std::cout << "[runways] drawing flat-color runway mesh\n";
        }
        if (rs == m_texturedShader && m_runwayTexture) {
            m_runwayTexture->bind(0);
            rs->setInt("uTexture", 0);
            rs->setBool("uUseUniformColor", false);
        } else {
            rs->setBool("uTerrainShading", false);
            rs->setBool("uTerrainUseTextures", false);
            rs->setBool("uTerrainUseMasks", false);
            rs->setBool("uUseUniformColor", true);
            rs->setVec3("uColor", m_runwayColor);
        }
        m_runwayMesh->draw();
    }
}

} // namespace nuage
