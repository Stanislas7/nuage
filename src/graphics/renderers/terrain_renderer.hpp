#pragma once

#include "math/mat4.hpp"
#include "math/vec3.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

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
    struct TileLevel {
        int level = 0;
        int width = 0;
        int height = 0;
        int tilesX = 0;
        int tilesY = 0;
        float pixelSizeX = 1.0f;
        float pixelSizeZ = 1.0f;
        std::string path;
    };

    struct TileLayer {
        std::string name;
        std::string format;
        std::vector<TileLevel> levels;
        float heightMin = 0.0f;
        float heightMax = 1.0f;
    };

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
    };

    struct HeightTile {
        int width = 0;
        int height = 0;
        std::vector<std::uint16_t> pixels;
    };

    void setupTiled(const std::string& configPath, AssetStore& assets);
    void setupProcedural(const std::string& configPath);
    void renderTiled(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);
    void renderProcedural(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);
    TileResource* ensureTileLoaded(const TileLevel& level, int x, int y);
    TileResource* ensureProceduralTileLoaded(int x, int y);
    bool loadHeightTileFile(const std::string& path, HeightTile& out);
    float bilinearSample(const HeightTile& tile, float x, float y) const;
    float sampleHeightAt(const TileLevel& heightLevel, float u, float v);
    float proceduralHeight(float worldX, float worldZ) const;
    Vec3 proceduralTileTint(int tileX, int tileY) const;

    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Shader* m_texturedShader = nullptr;
    Texture* m_texture = nullptr;
    bool m_textured = false;

    bool m_tiled = false;
    bool m_procedural = false;
    AssetStore* m_assets = nullptr;
    std::string m_manifestDir;
    TileLayer m_heightLayer;
    TileLayer m_albedoLayer;
    std::vector<int> m_albedoLevelForHeight;
    int m_tileSizePx = 512;
    float m_worldSizeX = 0.0f;
    float m_worldSizeZ = 0.0f;
    int m_tileMaxResolution = 128;
    bool m_tileFlipY = true;
    float m_tileViewDistance = 8000.0f;
    float m_tileLodDistance = 2000.0f;
    int m_tileMinLod = 0;
    int m_tileMaxLod = 0;
    int m_tileBudget = 256;
    int m_tileLoadsPerFrame = 32;
    int m_textureLoadsPerFrame = 32;
    int m_tilesLoadedThisFrame = 0;
    int m_texturesLoadedThisFrame = 0;
    std::unordered_map<std::string, TileResource> m_tileCache;

    std::unordered_map<std::string, HeightTile> m_heightTileCache;
    std::unordered_map<std::string, int> m_procTileCreateCounts;
    int m_procTileRebuilds = 0;

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
};

} // namespace nuage
