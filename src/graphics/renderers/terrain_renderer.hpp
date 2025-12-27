#pragma once

#include "math/mat4.hpp"
#include "math/vec3.hpp"
#include <unordered_map>
#include <string>
#include <vector>

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
        Mesh* mesh = nullptr;
        Texture* texture = nullptr;
        Vec3 center{0, 0, 0};
        float radius = 0.0f;
        int level = 0;
        int x = 0;
        int y = 0;
        bool textured = false;
    };

    void setupTiled(const std::string& configPath, AssetStore& assets);
    void renderTiled(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);
    TileResource* ensureTileLoaded(const TileLevel& level, int x, int y);

    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Shader* m_texturedShader = nullptr;
    Texture* m_texture = nullptr;
    bool m_textured = false;

    bool m_tiled = false;
    AssetStore* m_assets = nullptr;
    std::string m_manifestDir;
    TileLayer m_heightLayer;
    TileLayer m_albedoLayer;
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
    std::unordered_map<std::string, TileResource> m_tileCache;
};

} // namespace nuage
