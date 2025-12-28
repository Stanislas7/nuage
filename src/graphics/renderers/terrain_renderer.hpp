#pragma once

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
    void init(AssetStore& assets);
    void shutdown();
    void setup(const std::string& configPath, AssetStore& assets);
    void render(const Mat4& viewProjection, const Vec3& sunDir, const Vec3& cameraPos);
    bool isProcedural() const { return m_procedural; }
    bool isCompiled() const { return m_compiled; }
    int compiledVisibleRadius() const { return m_compiledVisibleRadius; }
    int compiledLoadsPerFrame() const { return m_compiledLoadsPerFrame; }
    int proceduralVisibleRadius() const { return m_procVisibleRadius; }
    int proceduralLoadsPerFrame() const { return m_procLoadsPerFrame; }
    void setCompiledVisibleRadius(int radius);
    void setCompiledLoadsPerFrame(int loads);
    void setProceduralVisibleRadius(int radius);
    void setProceduralLoadsPerFrame(int loads);
    TerrainVisualSettings& visuals() { return m_visuals; }
    const TerrainVisualSettings& visuals() const { return m_visuals; }
    void clampVisuals() { m_visuals.clamp(); }

private:
    struct TileResource {
        std::unique_ptr<Mesh> ownedMesh;
        std::unique_ptr<Mesh> ownedMeshLod1;
        Mesh* mesh = nullptr;
        Mesh* meshLod1 = nullptr;
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

    struct TerrainTextureSettings {
        bool enabled = false;
        float texScale = 0.02f;
        float detailScale = 0.08f;
        float detailStrength = 0.35f;
        float rockSlopeStart = 0.35f;
        float rockSlopeEnd = 0.7f;
        float rockStrength = 0.7f;
        Vec3 waterColor = Vec3(0.14f, 0.32f, 0.55f);
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
    void applyTextureConfig(const nlohmann::json& config, const std::string& configPath);
    void bindTerrainTextures(Shader* shader, bool useMasks) const;

    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Shader* m_texturedShader = nullptr;
    Texture* m_texture = nullptr;
    bool m_textured = false;
    TerrainTextureSettings m_textureSettings;
    Texture* m_texGrass = nullptr;
    Texture* m_texForest = nullptr;
    Texture* m_texRock = nullptr;
    Texture* m_texDirt = nullptr;
    Texture* m_texUrban = nullptr;

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
    float m_compiledLod1Distance = 0.0f;
    float m_compiledLod1DistanceSq = 0.0f;
    float m_compiledSkirtDepth = 0.0f;
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

    TerrainVisualSettings m_visuals;
};

} // namespace nuage
