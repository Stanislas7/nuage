#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/mesh.hpp"
#include "graphics/mesh_builder.hpp"
#include "graphics/lighting.hpp"
#include "utils/config_loader.hpp"
#include "utils/stb_image.h"
#include "math/vec2.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

namespace nuage {

void TerrainRenderer::init(AssetStore& assets) {
    m_shader = assets.getShader("basic");
    m_texturedShader = assets.getShader("textured");
    m_assets = &assets;
}

void TerrainRenderer::setup(const std::string& configPath, AssetStore& assets) {
    m_assets = &assets;
    m_tiled = false;
    m_procedural = false;
    m_compiled = false;
    m_tileCache.clear();
    m_heightTileCache.clear();
    m_procTileCreateCounts.clear();
    m_procTileRebuilds = 0;
    m_compiledTileCreateCounts.clear();
    m_compiledTileRebuilds = 0;
    m_heightLayer = {};
    m_albedoLayer = {};
    m_albedoLevelForHeight.clear();
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
    if (m_procedural) {
        renderProcedural(vp, sunDir, cameraPos);
        return;
    }
    if (m_compiled) {
        renderCompiled(vp, sunDir, cameraPos);
        return;
    }
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
    m_tileLoadsPerFrame = config.value("tileLoadsPerFrame", 32);
    m_textureLoadsPerFrame = config.value("tileTextureLoadsPerFrame", 32);

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
    if (m_textured) {
        m_albedoLevelForHeight.resize(m_heightLayer.levels.size(), -1);
        for (size_t i = 0; i < m_heightLayer.levels.size(); ++i) {
            const auto& h = m_heightLayer.levels[i];
            int best = -1;
            for (size_t j = 0; j < m_albedoLayer.levels.size(); ++j) {
                const auto& a = m_albedoLayer.levels[j];
                if (a.tilesX == h.tilesX && a.tilesY == h.tilesY) {
                    best = static_cast<int>(j);
                    break;
                }
            }
            m_albedoLevelForHeight[i] = best;
        }
    }
    m_tiled = true;
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

bool TerrainRenderer::loadHeightTileFile(const std::string& path, HeightTile& out) {
    stbi_set_flip_vertically_on_load(0);
    int width = 0;
    int height = 0;
    int channels = 0;
    std::uint16_t* data16 = stbi_load_16(path.c_str(), &width, &height, &channels, 1);
    if (!data16) {
        return false;
    }
    out.width = width;
    out.height = height;
    out.pixels.assign(data16, data16 + (width * height));
    stbi_image_free(data16);
    return true;
}

float TerrainRenderer::bilinearSample(const HeightTile& tile, float x, float y) const {
    if (tile.width <= 0 || tile.height <= 0) {
        return 0.0f;
    }
    float fx = std::clamp(x, 0.0f, static_cast<float>(tile.width - 1));
    float fy = std::clamp(y, 0.0f, static_cast<float>(tile.height - 1));

    int x0 = static_cast<int>(std::floor(fx));
    int y0 = static_cast<int>(std::floor(fy));
    int x1 = std::min(x0 + 1, tile.width - 1);
    int y1 = std::min(y0 + 1, tile.height - 1);

    float tx = fx - static_cast<float>(x0);
    float ty = fy - static_cast<float>(y0);

    auto at = [&](int px, int py) -> float {
        return static_cast<float>(tile.pixels[py * tile.width + px]);
    };

    float v00 = at(x0, y0);
    float v10 = at(x1, y0);
    float v01 = at(x0, y1);
    float v11 = at(x1, y1);

    float v0 = v00 + (v10 - v00) * tx;
    float v1 = v01 + (v11 - v01) * tx;
    return v0 + (v1 - v0) * ty;
}

float TerrainRenderer::sampleHeightAt(const TileLevel& heightLevel, float u, float v) {
    if (m_tileFlipY) {
        v = 1.0f - v;
    }
    float hX = u * std::max(1.0f, static_cast<float>(heightLevel.width - 1));
    float hY = v * std::max(1.0f, static_cast<float>(heightLevel.height - 1));

    int tileX = std::clamp(static_cast<int>(std::floor(hX / m_tileSizePx)), 0, heightLevel.tilesX - 1);
    int tileY = std::clamp(static_cast<int>(std::floor(hY / m_tileSizePx)), 0, heightLevel.tilesY - 1);

    std::filesystem::path tilePath = std::filesystem::path(m_manifestDir)
        / heightLevel.path / ("x" + std::to_string(tileX) + "_y" + std::to_string(tileY) + "." + m_heightLayer.format);

    auto it = m_heightTileCache.find(tilePath.string());
    if (it == m_heightTileCache.end()) {
        HeightTile tile;
        if (!loadHeightTileFile(tilePath.string(), tile)) {
            return m_heightLayer.heightMin;
        }
        it = m_heightTileCache.emplace(tilePath.string(), std::move(tile)).first;
    }

    const auto& tile = it->second;
    float localX = hX - tileX * m_tileSizePx;
    float localY = hY - tileY * m_tileSizePx;
    float raw = bilinearSample(tile, localX, localY) / 65535.0f;
    return m_heightLayer.heightMin + raw * (m_heightLayer.heightMax - m_heightLayer.heightMin);
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

    if (m_tilesLoadedThisFrame >= m_tileLoadsPerFrame) {
        return nullptr;
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

    std::filesystem::path albedoPath = std::filesystem::path(m_manifestDir)
        / level.path / ("x" + std::to_string(x) + "_y" + std::to_string(y) + "." + m_albedoLayer.format);
    bool useTexture = std::filesystem::exists(albedoPath);

    const auto& heightBase = m_heightLayer.levels.front();
    int tileResolution = std::max(2, m_tileMaxResolution >> level.level);
    int resX = tileResolution;
    int resZ = tileResolution;

    std::vector<float> verts;
    int stride = useTexture ? 8 : 9;
    verts.reserve((resX - 1) * (resZ - 1) * 6 * stride);

    auto heightAt = [&](float u, float v) -> float {
        return sampleHeightAt(heightBase, u, v);
    };

    auto appendVertex = [&](float px, float py, float pz, float nx, float ny, float nz, float u, float v) {
        if (useTexture) {
            verts.insert(verts.end(), {px, py, pz, nx, ny, nz, u, v});
        } else {
            float t = (py - m_heightLayer.heightMin) / (m_heightLayer.heightMax - m_heightLayer.heightMin);
            t = std::clamp(t, 0.0f, 1.0f);
            Vec3 c = heightColorLocal(t);
            verts.insert(verts.end(), {px, py, pz, nx, ny, nz, c.x, c.y, c.z});
        }
    };

    std::vector<Vec3> positions(resX * resZ);
    std::vector<Vec3> normals(resX * resZ, Vec3(0, 1, 0));
    std::vector<Vec2> uvs;
    if (useTexture) {
        uvs.resize(resX * resZ);
    }

    float albedoWidthMinus1 = std::max(1, level.width - 1);
    float albedoHeightMinus1 = std::max(1, level.height - 1);
    for (int z = 0; z < resZ; ++z) {
        for (int xIdx = 0; xIdx < resX; ++xIdx) {
            float fx = (resX > 1) ? static_cast<float>(xIdx) / (resX - 1) : 0.0f;
            float fz = (resZ > 1) ? static_cast<float>(z) / (resZ - 1) : 0.0f;

            float globalX = x * m_tileSizePx + fx * std::max(1, tileWidthPx - 1);
            float globalY = y * m_tileSizePx + fz * std::max(1, tileHeightPx - 1);

            float u = globalX / albedoWidthMinus1;
            float v = globalY / albedoHeightMinus1;

            float px = (u - 0.5f) * m_worldSizeX;
            float pz = (v - 0.5f) * m_worldSizeZ;
            float py = heightAt(u, v);

            int idx = z * resX + xIdx;
            positions[idx] = Vec3(px, py, pz);
            if (useTexture) {
                uvs[idx] = Vec2(u, v);
            }
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

    for (int z = 0; z < resZ - 1; ++z) {
        for (int xIdx = 0; xIdx < resX - 1; ++xIdx) {
            int i00 = z * resX + xIdx;
            int i10 = i00 + 1;
            int i01 = i00 + resX;
            int i11 = i01 + 1;

            if (useTexture) {
                const auto& uv00 = uvs[i00];
                const auto& uv10 = uvs[i10];
                const auto& uv01 = uvs[i01];
                const auto& uv11 = uvs[i11];

                appendVertex(positions[i00].x, positions[i00].y, positions[i00].z, normals[i00].x, normals[i00].y, normals[i00].z, uv00.x, uv00.y);
                appendVertex(positions[i10].x, positions[i10].y, positions[i10].z, normals[i10].x, normals[i10].y, normals[i10].z, uv10.x, uv10.y);
                appendVertex(positions[i11].x, positions[i11].y, positions[i11].z, normals[i11].x, normals[i11].y, normals[i11].z, uv11.x, uv11.y);

                appendVertex(positions[i00].x, positions[i00].y, positions[i00].z, normals[i00].x, normals[i00].y, normals[i00].z, uv00.x, uv00.y);
                appendVertex(positions[i11].x, positions[i11].y, positions[i11].z, normals[i11].x, normals[i11].y, normals[i11].z, uv11.x, uv11.y);
                appendVertex(positions[i01].x, positions[i01].y, positions[i01].z, normals[i01].x, normals[i01].y, normals[i01].z, uv01.x, uv01.y);
            } else {
                appendVertex(positions[i00].x, positions[i00].y, positions[i00].z, normals[i00].x, normals[i00].y, normals[i00].z, 0.0f, 0.0f);
                appendVertex(positions[i10].x, positions[i10].y, positions[i10].z, normals[i10].x, normals[i10].y, normals[i10].z, 0.0f, 0.0f);
                appendVertex(positions[i11].x, positions[i11].y, positions[i11].z, normals[i11].x, normals[i11].y, normals[i11].z, 0.0f, 0.0f);

                appendVertex(positions[i00].x, positions[i00].y, positions[i00].z, normals[i00].x, normals[i00].y, normals[i00].z, 0.0f, 0.0f);
                appendVertex(positions[i11].x, positions[i11].y, positions[i11].z, normals[i11].x, normals[i11].y, normals[i11].z, 0.0f, 0.0f);
                appendVertex(positions[i01].x, positions[i01].y, positions[i01].z, normals[i01].x, normals[i01].y, normals[i01].z, 0.0f, 0.0f);
            }
        }
    }

    auto terrainData = std::move(verts);
    if (terrainData.empty()) {
        return nullptr;
    }

    std::string meshName = "terrain_tile_" + key;
    if (useTexture) {
        m_assets->loadTexturedMesh(meshName, terrainData);
    } else {
        m_assets->loadMesh(meshName, terrainData);
    }
    m_tilesLoadedThisFrame += 1;

    Texture* texture = nullptr;
    if (useTexture) {
        if (m_texturesLoadedThisFrame >= m_textureLoadsPerFrame) {
            useTexture = false;
        }
    }
    if (useTexture) {
        std::string texName = "terrain_tile_tex_" + key;
        if (m_assets->loadTexture(texName, albedoPath.string())) {
            texture = m_assets->getTexture(texName);
        }
        m_texturesLoadedThisFrame += 1;
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

void TerrainRenderer::renderTiled(const Mat4& vp, const Vec3& sunDir, const Vec3& cameraPos) {
    if (m_heightLayer.levels.empty()) {
        return;
    }
    m_tilesLoadedThisFrame = 0;
    m_texturesLoadedThisFrame = 0;
    const auto& renderLevels = (!m_albedoLayer.levels.empty()) ? m_albedoLayer.levels : m_heightLayer.levels;
    int maxLevel = std::clamp(m_tileMaxLod, 0, static_cast<int>(renderLevels.size() - 1));
    int minLevel = std::clamp(m_tileMinLod, 0, maxLevel);
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

    for (int levelIdx = minLevel; levelIdx <= maxLevel; ++levelIdx) {
        const auto& level = renderLevels[levelIdx];
        if (level.tilesX <= 0 || level.tilesY <= 0) {
            continue;
        }
        float baseTileWorldX = m_tileSizePx * level.pixelSizeX;
        float baseTileWorldZ = m_tileSizePx * level.pixelSizeZ;

        float minDist = (levelIdx == minLevel)
            ? 0.0f
            : m_tileLodDistance * std::pow(2.0f, levelIdx - minLevel - 1);
        float maxDist = m_tileLodDistance * std::pow(2.0f, levelIdx - minLevel);
        if (maxLevel <= minLevel || levelIdx == maxLevel) {
            maxDist = m_tileViewDistance;
        }

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
