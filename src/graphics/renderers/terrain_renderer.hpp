#pragma once

#include "math/geo.hpp"
#include "math/mat4.hpp"
#include "math/vec3.hpp"
#include "graphics/renderers/terrain/terrain_visual_settings.hpp"
#include "utils/json.hpp"
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
    struct TerrainTextureSettings {
        bool enabled = false;
        float texScale = 0.02f;
        float detailScale = 0.08f;
        float detailStrength = 0.35f;
        float rockSlopeStart = 0.35f;
        float rockSlopeEnd = 0.7f;
        float rockStrength = 0.7f;
        float macroScale = 0.0012f;
        float macroStrength = 0.25f;
        float megaScale = 0.00035f;
        float megaStrength = 0.12f;
        float farmlandStrength = 0.5f;
        float farmlandStripeScale = 0.004f;
        float farmlandStripeContrast = 0.6f;
        float scrubStrength = 0.25f;
        float scrubNoiseScale = 0.0016f;
        Vec3 grassTintA = Vec3(0.75f, 0.95f, 0.65f);
        Vec3 grassTintB = Vec3(0.55f, 0.7f, 0.45f);
        float grassTintStrength = 0.35f;
        Vec3 forestTintA = Vec3(0.7f, 0.85f, 0.6f);
        Vec3 forestTintB = Vec3(0.5f, 0.65f, 0.45f);
        float forestTintStrength = 0.25f;
        Vec3 urbanTintA = Vec3(0.95f, 0.95f, 0.95f);
        Vec3 urbanTintB = Vec3(0.75f, 0.78f, 0.8f);
        float urbanTintStrength = 0.2f;
        float microScale = 0.22f;
        float microStrength = 0.18f;
        float waterDetailScale = 0.08f;
        float waterDetailStrength = 0.25f;
        Vec3 waterColor = Vec3(0.14f, 0.32f, 0.55f);
        float maskFeatherMeters = 42.0f;
        float maskJitterMeters = 18.0f;
        float maskEdgeNoise = 0.35f;
        float shoreWidth = 0.45f;
        float shoreFeather = 0.18f;
        float wetStrength = 0.35f;
        float farmTexScale = 0.12f;
        float roadStrength = 0.7f;
    };

    struct TerrainSample {
        float height = 0.0f;
        Vec3 normal = Vec3(0.0f, 1.0f, 0.0f);
        float water = 0.0f;
        float urban = 0.0f;
        float forest = 0.0f;
        bool onRunway = false;
    };

    void init(AssetStore& assets);
    void shutdown();
    void setup(const std::string& configPath, AssetStore& assets);
    void render(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);
    bool isCompiled() const { return m_compiled; }
    bool treesEnabled() const { return m_treesEnabled; }
    bool hasCompiledOrigin() const { return m_compiledOriginValid; }
    GeoOrigin compiledOrigin() const { return m_compiledOrigin; }
    Vec3 compiledGeoToWorld(double latDeg, double lonDeg, double altMeters) const;
    bool sampleSurface(float worldX, float worldZ, TerrainSample& outSample) const;
    bool sampleSurfaceNoLoad(float worldX, float worldZ, TerrainSample& outSample) const;
    bool sampleHeight(float worldX, float worldZ, float& outHeight) const;
    bool sampleSurfaceHeight(float worldX, float worldZ, float& outHeight) const;
    bool sampleSurfaceHeightNoLoad(float worldX, float worldZ, float& outHeight) const;
    void preloadPhysicsAt(float worldX, float worldZ, int radius = 0);
    int compiledVisibleRadius() const { return m_compiledVisibleRadius; }
    int compiledLoadsPerFrame() const { return m_compiledLoadsPerFrame; }
    void setCompiledVisibleRadius(int radius);
    void setCompiledLoadsPerFrame(int loads);
    void setTreesEnabled(bool enabled);
    bool debugMaskView() const { return m_debugMaskView; }
    void setDebugMaskView(bool enabled) { m_debugMaskView = enabled; }
    TerrainVisualSettings& visuals() { return m_visuals; }
    const TerrainVisualSettings& visuals() const { return m_visuals; }
    TerrainTextureSettings& textureSettings() { return m_textureSettings; }
    const TerrainTextureSettings& textureSettings() const { return m_textureSettings; }
    void clampVisuals() { m_visuals.clamp(); }

private:
    struct TileResource {
        std::unique_ptr<Mesh> ownedMesh;
        std::unique_ptr<Mesh> ownedMeshLod1;
        std::unique_ptr<Mesh> ownedTreeMesh;
        Mesh* mesh = nullptr;
        Mesh* meshLod1 = nullptr;
        Mesh* treeMesh = nullptr;
        Texture* texture = nullptr;
        std::unique_ptr<Texture> ownedMaskTexture;
        Texture* maskTexture = nullptr;
        Vec3 center{0, 0, 0};
        float radius = 0.0f;
        float tileMinX = 0.0f;
        float tileMinZ = 0.0f;
        int level = 0;
        int x = 0;
        int y = 0;
        int gridRes = 0;
        bool textured = false;
        bool compiled = false;
        bool hasGrid = false;
        std::vector<float> gridVerts;
    };

    void setupCompiled(const std::string& configPath);
    void renderCompiled(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);
    TileResource* ensureCompiledTileLoaded(int x, int y, bool force = false);
    std::int64_t packedTileKey(int x, int y) const;
    void applyTextureConfig(const nlohmann::json& config, const std::string& configPath);
    void bindTerrainTextures(Shader* shader, bool useMasks) const;
    void loadRunways(const nlohmann::json& config, const std::string& configPath);
    bool sampleRunway(float worldX, float worldZ, TerrainSample& outSample) const;
    bool sampleCompiledSurface(int tx, int ty, float worldX, float worldZ,
                               bool forceLoad, TerrainSample& outSample) const;
    bool sampleCompiledSurfaceCached(int tx, int ty, float worldX, float worldZ,
                                     TerrainSample& outSample) const;

    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Shader* m_texturedShader = nullptr;
    TerrainTextureSettings m_textureSettings;
    Texture* m_texGrass = nullptr;
    Texture* m_texGrassB = nullptr;
    Texture* m_texForest = nullptr;
    Texture* m_texRock = nullptr;
    Texture* m_texDirt = nullptr;
    Texture* m_texDirtB = nullptr;
    Texture* m_texUrban = nullptr;
    Texture* m_texGrassNormal = nullptr;
    Texture* m_texDirtNormal = nullptr;
    Texture* m_texRockNormal = nullptr;
    Texture* m_texUrbanNormal = nullptr;
    Texture* m_texGrassRough = nullptr;
    Texture* m_texDirtRough = nullptr;
    Texture* m_texRockRough = nullptr;
    Texture* m_texUrbanRough = nullptr;

    std::unique_ptr<Mesh> m_runwayMesh;
    bool m_runwaysEnabled = false;
    Vec3 m_runwayColor = Vec3(0.12f, 0.12f, 0.12f);
    float m_runwayHeightOffset = 0.15f;
    Texture* m_runwayTexture = nullptr;

    struct RunwayCollider {
        Vec3 center;
        Vec3 dir;
        Vec3 perp;
        float halfLength = 0.0f;
        float halfWidth = 0.0f;
        float h0 = 0.0f;
        float h1 = 0.0f;
    };
    std::vector<RunwayCollider> m_runwayColliders;

    bool m_compiled = false;
    bool m_debugMaskView = false;
    AssetStore* m_assets = nullptr;
    std::unordered_map<std::string, TileResource> m_tileCache;

    std::unordered_map<std::string, int> m_compiledTileCreateCounts;
    int m_compiledTileRebuilds = 0;

    std::string m_compiledManifestDir;
    float m_compiledTileSizeMeters = 2000.0f;
    int m_compiledGridResolution = 129;
    int m_compiledVisibleRadius = 1;
    int m_compiledLoadsPerFrame = 2;
    bool m_compiledDebugLog = true;
    float m_compiledLod1Distance = 0.0f;
    float m_compiledLod1DistanceSq = 0.0f;
    float m_compiledSkirtDepth = 0.0f;
    GeoOrigin m_compiledOrigin;
    bool m_compiledOriginValid = false;
    int m_compiledMaskResolution = 0;
    std::unordered_set<std::int64_t> m_compiledTiles;
    int m_compiledTilesLoadedThisFrame = 0;

    bool m_treesEnabled = false;
    float m_treesDensityPerSqKm = 80.0f;
    float m_treesMinHeight = 4.0f;
    float m_treesMaxHeight = 10.0f;
    float m_treesMinRadius = 0.8f;
    float m_treesMaxRadius = 2.2f;
    float m_treesMaxSlope = 0.7f;
    float m_treesMaxDistance = 5000.0f;
    float m_treesMaxDistanceSq = 0.0f;
    bool m_treesAvoidRoads = true;
    int m_treesSeed = 1337;

    TerrainVisualSettings m_visuals;
};

} // namespace nuage
