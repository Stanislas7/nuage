#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/glad.h"
#include "graphics/lighting.hpp"
#include "graphics/mesh.hpp"
#include "graphics/renderers/terrain/terrain_mask_blend.hpp"
#include "graphics/renderers/terrain/terrain_tile_io.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "math/vec2.hpp"
#include "utils/config_loader.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace nuage {

namespace {
constexpr float kSqMetersPerSqKm = 1000000.0f;

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
                                           const std::vector<std::uint8_t>* maskData, int maskRes,
                                           bool avoidRoads, bool enabled, float densityPerSqKm, float minHeight,
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
        if (avoidRoads && maskData && maskRes > 1) {
            float fx = std::clamp((x - tileMinX) / tileSize, 0.0f, 1.0f);
            float fz = std::clamp((z - tileMinZ) / tileSize, 0.0f, 1.0f);
            int mx = static_cast<int>(std::round(fx * (maskRes - 1)));
            int mz = static_cast<int>(std::round(fz * (maskRes - 1)));
            mx = std::clamp(mx, 0, maskRes - 1);
            mz = std::clamp(mz, 0, maskRes - 1);
            std::uint8_t cls = (*maskData)[static_cast<size_t>(mz) * maskRes + static_cast<size_t>(mx)];
            if (cls == 7) {
                continue;
            }
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
        m_treesAvoidRoads = trees.value("avoidRoads", m_treesAvoidRoads);
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

    m_compiled = true;
}

Vec3 TerrainRenderer::compiledGeoToWorld(double latDeg, double lonDeg, double altMeters) const {
    if (!m_compiledOriginValid) {
        return Vec3(0.0f, 0.0f, 0.0f);
    }
    return llaToEnu(m_compiledOrigin, latDeg, lonDeg, altMeters);
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

std::int64_t TerrainRenderer::packedTileKey(int x, int y) const {
    return (static_cast<std::int64_t>(x) << 32) ^ (static_cast<std::uint32_t>(y));
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
        const std::vector<std::uint8_t>* roadMask = maskData.empty() ? nullptr : &maskData;
        resource.ownedTreeMesh = buildTreeMeshForTile(resource.gridVerts.empty() ? gridVerts : resource.gridVerts,
                                                      res, x, y,
                                                      tileMinX, tileMinZ,
                                                      m_compiledTileSizeMeters, useWaterMask,
                                                      roadMask, m_compiledMaskResolution, m_treesAvoidRoads,
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
        glDisable(GL_DEPTH_TEST);
        m_runwayMesh->draw();
        glEnable(GL_DEPTH_TEST);
    }
}

} // namespace nuage
