#include "graphics/renderers/terrain/terrain_visual_settings.hpp"
#include "graphics/shader.hpp"
#include <algorithm>

namespace nuage {
namespace {
float smoothstepLocal(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Vec3 mixColor(const Vec3& a, const Vec3& b, float t) {
    return a * (1.0f - t) + b * t;
}
} // namespace

void TerrainVisualSettings::resetDefaults() {
    heightMin = 0.0f;
    heightMax = 1500.0f;
    noiseScale = 0.002f;
    noiseStrength = 0.3f;
    slopeStart = 0.3f;
    slopeEnd = 0.7f;
    slopeDarken = 0.3f;
    fogDistance = 12000.0f;
    desaturate = 0.2f;
    tint = Vec3(0.45f, 0.52f, 0.33f);
    tintStrength = 0.15f;
    distanceDesatStart = 3000.0f;
    distanceDesatEnd = 12000.0f;
    distanceDesatStrength = 0.35f;
    distanceContrastLoss = 0.25f;
    fogSunScale = 0.35f;
}

void TerrainVisualSettings::setHeightRange(float minHeight, float maxHeight) {
    heightMin = minHeight;
    heightMax = maxHeight;
}

void TerrainVisualSettings::applyConfig(const nlohmann::json& config) {
    if (!config.contains("terrainVisuals") || !config["terrainVisuals"].is_object()) {
        return;
    }
    const auto& visuals = config["terrainVisuals"];
    heightMin = visuals.value("heightMin", heightMin);
    heightMax = visuals.value("heightMax", heightMax);
    noiseScale = visuals.value("noiseScale", noiseScale);
    noiseStrength = visuals.value("noiseStrength", noiseStrength);
    slopeStart = visuals.value("slopeStart", slopeStart);
    slopeEnd = visuals.value("slopeEnd", slopeEnd);
    slopeDarken = visuals.value("slopeDarken", slopeDarken);
    fogDistance = visuals.value("fogDistance", fogDistance);
    desaturate = visuals.value("desaturate", desaturate);
    tintStrength = visuals.value("tintStrength", tintStrength);
    distanceDesatStart = visuals.value("distanceDesatStart", distanceDesatStart);
    distanceDesatEnd = visuals.value("distanceDesatEnd", distanceDesatEnd);
    distanceDesatStrength = visuals.value("distanceDesatStrength", distanceDesatStrength);
    distanceContrastLoss = visuals.value("distanceContrastLoss", distanceContrastLoss);
    fogSunScale = visuals.value("fogSunScale", fogSunScale);
    if (visuals.contains("tint") && visuals["tint"].is_array() && visuals["tint"].size() == 3) {
        tint = Vec3(
            visuals["tint"][0].get<float>(),
            visuals["tint"][1].get<float>(),
            visuals["tint"][2].get<float>()
        );
    }
}

void TerrainVisualSettings::clamp() {
    if (heightMax <= heightMin) {
        heightMax = heightMin + 1.0f;
    }
    fogDistance = std::max(1.0f, fogDistance);
    distanceDesatStart = std::max(0.0f, distanceDesatStart);
    distanceDesatEnd = std::max(distanceDesatStart + 1.0f, distanceDesatEnd);
    distanceDesatStrength = std::clamp(distanceDesatStrength, 0.0f, 1.0f);
    distanceContrastLoss = std::clamp(distanceContrastLoss, 0.0f, 1.0f);
    fogSunScale = std::clamp(fogSunScale, 0.0f, 0.95f);
}

Vec3 TerrainVisualSettings::fogColorForSunDir(const Vec3& sunDir) const {
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

float TerrainVisualSettings::fogDistanceForSunDir(const Vec3& sunDir) const {
    float elevation = std::clamp(sunDir.normalized().y, -1.0f, 1.0f);
    float dayFactor = smoothstepLocal(-0.25f, 0.35f, elevation);
    float fogScale = 1.0f - fogSunScale * (1.0f - dayFactor);
    return fogDistance * std::max(0.15f, fogScale);
}

void TerrainVisualSettings::bind(Shader* shader, const Vec3& sunDir, const Vec3& cameraPos) const {
    if (!shader) {
        return;
    }
    shader->setBool("uTerrainShading", true);
    shader->setVec3("uCameraPos", cameraPos);
    shader->setVec3("uTerrainFogColor", fogColorForSunDir(sunDir));
    shader->setFloat("uTerrainHeightMin", heightMin);
    shader->setFloat("uTerrainHeightMax", heightMax);
    shader->setFloat("uTerrainNoiseScale", noiseScale);
    shader->setFloat("uTerrainNoiseStrength", noiseStrength);
    shader->setFloat("uTerrainSlopeStart", slopeStart);
    shader->setFloat("uTerrainSlopeEnd", slopeEnd);
    shader->setFloat("uTerrainSlopeDarken", slopeDarken);
    shader->setFloat("uTerrainFogDistance", fogDistanceForSunDir(sunDir));
    shader->setFloat("uTerrainDesaturate", desaturate);
    shader->setVec3("uTerrainTint", tint);
    shader->setFloat("uTerrainTintStrength", tintStrength);
    shader->setFloat("uTerrainDistanceDesatStart", distanceDesatStart);
    shader->setFloat("uTerrainDistanceDesatEnd", distanceDesatEnd);
    shader->setFloat("uTerrainDistanceDesatStrength", distanceDesatStrength);
    shader->setFloat("uTerrainDistanceContrastLoss", distanceContrastLoss);
}

} // namespace nuage
