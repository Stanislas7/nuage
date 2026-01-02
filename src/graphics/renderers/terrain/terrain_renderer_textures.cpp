#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include <filesystem>
#include <iostream>

namespace nuage {

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
    m_textureSettings.megaScale = texConfig.value("megaScale", m_textureSettings.megaScale);
    m_textureSettings.megaStrength = texConfig.value("megaStrength", m_textureSettings.megaStrength);
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
    m_textureSettings.maskFeatherMeters = texConfig.value("maskFeatherMeters", m_textureSettings.maskFeatherMeters);
    m_textureSettings.maskJitterMeters = texConfig.value("maskJitterMeters", m_textureSettings.maskJitterMeters);
    m_textureSettings.maskEdgeNoise = texConfig.value("maskEdgeNoise", m_textureSettings.maskEdgeNoise);
    m_textureSettings.shoreWidth = texConfig.value("shoreWidth", m_textureSettings.shoreWidth);
    m_textureSettings.shoreFeather = texConfig.value("shoreFeather", m_textureSettings.shoreFeather);
    m_textureSettings.wetStrength = texConfig.value("wetStrength", m_textureSettings.wetStrength);
    m_textureSettings.farmTexScale = texConfig.value("farmTexScale", m_textureSettings.farmTexScale);
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
    loadTex("grassB", m_texGrassB, "terrain_grass_b");
    loadTex("grassC", m_texGrassC, "terrain_grass_c");
    loadedAny |= loadTex("forest", m_texForest, "terrain_forest");
    loadedAny |= loadTex("rock", m_texRock, "terrain_rock");
    loadedAny |= loadTex("dirt", m_texDirt, "terrain_dirt");
    loadedAny |= loadTex("urban", m_texUrban, "terrain_urban");
    loadTex("rockB", m_texRockB, "terrain_rock_b");
    loadTex("dirtB", m_texDirtB, "terrain_dirt_b");
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

void TerrainRenderer::bindTerrainTextures(Shader* shader, bool useMasks) const {
    if (!shader) {
        return;
    }

    Texture* grass = m_texGrass;
    Texture* grassB = m_texGrassB ? m_texGrassB : grass;
    Texture* grassC = m_texGrassC ? m_texGrassC : grass;
    Texture* forest = m_texForest ? m_texForest : grass;
    Texture* rock = m_texRock ? m_texRock : grass;
    Texture* rockB = m_texRockB ? m_texRockB : rock;
    Texture* dirt = m_texDirt ? m_texDirt : grass;
    Texture* dirtB = m_texDirtB ? m_texDirtB : dirt;
    Texture* urban = m_texUrban ? m_texUrban : (dirt ? dirt : grass);
    bool enabled = m_textureSettings.enabled && grass && rock && urban;
    shader->setBool("uTerrainUseTextures", enabled);
    shader->setBool("uTerrainUseMasks", useMasks);
    shader->setBool("uTerrainDebugMaskView", m_debugMaskView);
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
    shader->setFloat("uTerrainMegaScale", m_textureSettings.megaScale);
    shader->setFloat("uTerrainMegaStrength", m_textureSettings.megaStrength);
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
    shader->setFloat("uTerrainMaskFeatherMeters", m_textureSettings.maskFeatherMeters);
    shader->setFloat("uTerrainMaskJitterMeters", m_textureSettings.maskJitterMeters);
    shader->setFloat("uTerrainMaskEdgeNoise", m_textureSettings.maskEdgeNoise);
    shader->setFloat("uTerrainShoreWidth", m_textureSettings.shoreWidth);
    shader->setFloat("uTerrainShoreFeather", m_textureSettings.shoreFeather);
    shader->setFloat("uTerrainWetStrength", m_textureSettings.wetStrength);
    shader->setFloat("uTerrainFarmTexScale", m_textureSettings.farmTexScale);

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
    grassB->bind(14);
    shader->setInt("uTerrainTexGrassB", 14);
    grassC->bind(15);
    shader->setInt("uTerrainTexGrassC", 15);
    rockB->bind(16);
    shader->setInt("uTerrainTexRockB", 16);
    dirtB->bind(17);
    shader->setInt("uTerrainTexDirtB", 17);
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

} // namespace nuage
