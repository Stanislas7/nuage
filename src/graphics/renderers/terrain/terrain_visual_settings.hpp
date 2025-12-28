#pragma once

#include "math/vec3.hpp"
#include "utils/json.hpp"

namespace nuage {

class Shader;

struct TerrainVisualSettings {
    float heightMin = 0.0f;
    float heightMax = 1500.0f;
    float noiseScale = 0.002f;
    float noiseStrength = 0.3f;
    float slopeStart = 0.3f;
    float slopeEnd = 0.7f;
    float slopeDarken = 0.3f;
    float fogDistance = 12000.0f;
    float desaturate = 0.2f;
    Vec3 tint = Vec3(0.45f, 0.52f, 0.33f);
    float tintStrength = 0.15f;
    float distanceDesatStart = 3000.0f;
    float distanceDesatEnd = 12000.0f;
    float distanceDesatStrength = 0.35f;
    float distanceContrastLoss = 0.25f;
    float fogSunScale = 0.35f;

    void resetDefaults();
    void setHeightRange(float minHeight, float maxHeight);
    void applyConfig(const nlohmann::json& config);
    void clamp();
    Vec3 fogColorForSunDir(const Vec3& sunDir) const;
    float fogDistanceForSunDir(const Vec3& sunDir) const;
    void bind(Shader* shader, const Vec3& sunDir, const Vec3& cameraPos) const;
};

} // namespace nuage
