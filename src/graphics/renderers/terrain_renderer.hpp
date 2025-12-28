#pragma once

#include "math/mat4.hpp"
#include "math/vec3.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_set>

namespace nuage {

class AssetStore;
class Mesh;
class Shader;
class Texture;

/**
 * @brief Handles terrain mesh management and rendering for a session.
 */
class TerrainRenderer {
public:
    void init(AssetStore& assets);
    void setup(const std::string& configPath, AssetStore& assets);
    void render(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);

private:
    struct TileResource {
        std::unique_ptr<Mesh> ownedMesh;
        Mesh* mesh = nullptr;
        Texture* texture = nullptr;
        Vec3 center{0, 0, 0};
        float radius = 0.0f;
        int level = 0;
        int x = 0;
        int y = 0;
        bool textured = false;
        bool procedural = false;
        bool compiled = false;
    };

    void setupProcedural(const std::string& configPath);
    void setupCompiled(const std::string& configPath);
    void renderProcedural(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);
    void renderCompiled(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);
    TileResource* ensureProceduralTileLoaded(int x, int y);
    TileResource* ensureCompiledTileLoaded(int x, int y);
    float proceduralHeight(float worldX, float worldZ) const;
    Vec3 proceduralTileTint(int tileX, int tileY) const;
    std::int64_t packedTileKey(int x, int y) const;
    bool loadCompiledMesh(const std::string& path, std::vector<float>& out) const;
    bool loadCompiledMask(const std::string& path, int expectedRes, std::vector<std::uint8_t>& out) const;
    void applyMaskToVerts(std::vector<float>& verts, const std::vector<std::uint8_t>& mask,
                          int maskRes, float tileMinX, float tileMinZ) const;
    Vec3 maskClassColor(std::uint8_t cls) const;

    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Shader* m_texturedShader = nullptr;
    Texture* m_texture = nullptr;
    bool m_textured = false;

    bool m_procedural = false;
    bool m_compiled = false;
    AssetStore* m_assets = nullptr;
    std::unordered_map<std::string, TileResource> m_tileCache;

    std::unordered_map<std::string, int> m_procTileCreateCounts;
    int m_procTileRebuilds = 0;
    std::unordered_map<std::string, int> m_compiledTileCreateCounts;
    int m_compiledTileRebuilds = 0;

    std::string m_compiledManifestDir;
    float m_compiledTileSizeMeters = 2000.0f;
    int m_compiledGridResolution = 129;
    int m_compiledVisibleRadius = 1;
    int m_compiledLoadsPerFrame = 2;
    bool m_compiledDebugLog = true;
    float m_compiledMinX = 0.0f;
    float m_compiledMinZ = 0.0f;
    float m_compiledMaxX = 0.0f;
    float m_compiledMaxZ = 0.0f;
    int m_compiledMaskResolution = 0;
    std::unordered_set<std::int64_t> m_compiledTiles;
    int m_compiledTilesLoadedThisFrame = 0;

    float m_procTileSizeMeters = 2000.0f;
    int m_procGridResolution = 129;
    int m_procVisibleRadius = 1;
    int m_procLoadsPerFrame = 2;
    float m_procHeightAmplitude = 250.0f;
    float m_procHeightBase = 0.0f;
    float m_procFrequency = 0.0006f;
    float m_procFrequency2 = 0.0013f;
    int m_procSeed = 1337;
    float m_procBorderWidth = 0.03f;
    bool m_procDebugBorders = true;
    bool m_procDebugLog = true;
    int m_procTilesLoadedThisFrame = 0;

    float m_terrainHeightMin = 0.0f;
    float m_terrainHeightMax = 1500.0f;
    float m_terrainNoiseScale = 0.002f;
    float m_terrainNoiseStrength = 0.3f;
    float m_terrainSlopeStart = 0.3f;
    float m_terrainSlopeEnd = 0.7f;
    float m_terrainSlopeDarken = 0.3f;
    float m_terrainFogDistance = 12000.0f;
    float m_terrainDesaturate = 0.2f;
    Vec3 m_terrainTint = Vec3(0.45f, 0.52f, 0.33f);
    float m_terrainTintStrength = 0.15f;
};

} // namespace nuage
