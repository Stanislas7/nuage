#include "graphics/renderers/terrain_renderer.hpp"
#include "graphics/asset_store.hpp"
#include "graphics/mesh.hpp"
#include "graphics/texture.hpp"
#include "math/vec2.hpp"
#include "utils/config_loader.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace nuage {

namespace {
constexpr double kFtToM = 0.3048;

bool parseNumber(const nlohmann::json& value, double& out) {
    if (value.is_number_float() || value.is_number_integer() || value.is_number_unsigned()) {
        out = value.get<double>();
        return true;
    }
    if (value.is_string()) {
        const std::string s = value.get<std::string>();
        if (s.empty()) return false;
        char* end = nullptr;
        out = std::strtod(s.c_str(), &end);
        return end != s.c_str();
    }
    return false;
}
}

void TerrainRenderer::loadRunways(const nlohmann::json& config, const std::string& configPath) {
    m_runwayMesh.reset();
    m_runwaysEnabled = false;
    m_runwayColliders.clear();
    m_runwayTexture = nullptr;

    if (!config.contains("runways") || !config["runways"].is_object()) {
        return;
    }
    const auto& runwaysConfig = config["runways"];
    m_runwaysEnabled = runwaysConfig.value("enabled", true);
    if (!m_runwaysEnabled) {
        return;
    }
    bool snapToTerrain = runwaysConfig.value("snapToTerrain", true);
    auto sampleTerrainHeight = [&](float x, float z, float& out) -> bool {
        if (!m_compiledTiles.empty()) {
            int tx = static_cast<int>(std::floor(x / m_compiledTileSizeMeters));
            int ty = static_cast<int>(std::floor(z / m_compiledTileSizeMeters));
            TerrainSample sample;
            if (sampleCompiledSurface(tx, ty, x, z, true, sample)) {
                out = sample.height;
                return true;
            }
        }
        return false;
    };
    float runwayTexScaleU = runwaysConfig.value("textureScaleU", 5.0f);
    float runwayTexScaleV = runwaysConfig.value("textureScaleV", 30.0f);

    std::filesystem::path cfg(configPath);
    auto resolve = [&](const std::string& p) {
        if (p.empty()) return p;
        std::filesystem::path path(p);
        if (path.is_absolute()) return p;
        return (cfg.parent_path() / path).string();
    };

    std::string runwayTexPath = resolve(runwaysConfig.value("texture", ""));
    if (!runwayTexPath.empty() && m_assets) {
        if (m_assets->loadTexture("runway_tex", runwayTexPath, true)) {
            m_runwayTexture = m_assets->getTexture("runway_tex");
        }
    }

    std::string runwaysPath = resolve(runwaysConfig.value("json", ""));
    if (runwaysPath.empty()) {
        return;
    }

    auto runwaysOpt = loadJsonConfig(runwaysPath);
    if (!runwaysOpt) {
        std::cerr << "Failed to load runways JSON: " << runwaysPath << "\n";
        return;
    }
    const auto& runways = *runwaysOpt;

    if (runwaysConfig.contains("color") && runwaysConfig["color"].is_array()
        && runwaysConfig["color"].size() == 3) {
        m_runwayColor = Vec3(runwaysConfig["color"][0].get<float>(),
                             runwaysConfig["color"][1].get<float>(),
                             runwaysConfig["color"][2].get<float>());
    }
    m_runwayHeightOffset = std::max(0.0f, runwaysConfig.value("heightOffset", m_runwayHeightOffset));

    std::vector<float> verts;
    if (runways.contains("runways") && runways["runways"].is_array()) {
        for (const auto& runway : runways["runways"]) {
            if (!runway.is_object()) {
                continue;
            }
            std::string closed = runway.value("closed", "");
            if (closed == "1" || closed == "true" || closed == "TRUE") {
                continue;
            }
            if (!runway.contains("leENU") || !runway.contains("heENU")) {
                continue;
            }
            const auto& le = runway["leENU"];
            const auto& he = runway["heENU"];
            if (!le.is_array() || !he.is_array() || le.size() != 3 || he.size() != 3) {
                continue;
            }

            double widthFt = 0.0;
            if (!runway.contains("widthFt") || !parseNumber(runway["widthFt"], widthFt)) {
                continue;
            }
            float widthMeters = static_cast<float>(widthFt * kFtToM);
            if (widthMeters <= 0.1f) {
                continue;
            }

            Vec3 lePos(le[0].get<float>(), le[1].get<float>(), le[2].get<float>());
            Vec3 hePos(he[0].get<float>(), he[1].get<float>(), he[2].get<float>());
            if (snapToTerrain) {
                float groundY = 0.0f;
                if (sampleTerrainHeight(lePos.x, lePos.z, groundY)) {
                    lePos.y = groundY;
                }
                if (sampleTerrainHeight(hePos.x, hePos.z, groundY)) {
                    hePos.y = groundY;
                }
            }

            float dx = hePos.x - lePos.x;
            float dz = hePos.z - lePos.z;
            float length = std::sqrt(dx * dx + dz * dz);
            if (length < 1.0f) {
                continue;
            }
            Vec3 dir(dx / length, 0.0f, dz / length);
            Vec3 perp(-dir.z, 0.0f, dir.x);
            float halfWidth = widthMeters * 0.5f;
            float halfLength = length * 0.5f;

            RunwayCollider collider;
            collider.center = (lePos + hePos) * 0.5f;
            collider.dir = dir;
            collider.perp = perp;
            collider.halfLength = halfLength;
            collider.halfWidth = halfWidth;
            collider.h0 = lePos.y;
            collider.h1 = hePos.y;
            m_runwayColliders.push_back(collider);

            Vec3 leOffset = perp * halfWidth;
            Vec3 heOffset = perp * halfWidth;

            Vec3 p0(lePos.x + leOffset.x, lePos.y + m_runwayHeightOffset, lePos.z + leOffset.z);
            Vec3 p1(lePos.x - leOffset.x, lePos.y + m_runwayHeightOffset, lePos.z - leOffset.z);
            Vec3 p2(hePos.x - heOffset.x, hePos.y + m_runwayHeightOffset, hePos.z - heOffset.z);
            Vec3 p3(hePos.x + heOffset.x, hePos.y + m_runwayHeightOffset, hePos.z + heOffset.z);
            float uvU0 = (widthMeters * 0.5f) / std::max(0.1f, runwayTexScaleU);
            float uvU1 = -uvU0;
            float uvV0 = 0.0f;
            float uvV1 = length / std::max(0.1f, runwayTexScaleV);

            Vec3 normal(0.0f, 1.0f, 0.0f);
            auto push = [&](const Vec3& pos, const Vec2& uv) {
                verts.insert(verts.end(), {
                    pos.x, pos.y, pos.z,
                    normal.x, normal.y, normal.z,
                    uv.x, uv.y
                });
            };

            push(p0, Vec2(uvU0, uvV0));
            push(p1, Vec2(uvU1, uvV0));
            push(p2, Vec2(uvU1, uvV1));

            push(p0, Vec2(uvU0, uvV0));
            push(p2, Vec2(uvU1, uvV1));
            push(p3, Vec2(uvU0, uvV1));
        }
    }

    if (!verts.empty()) {
        m_runwayMesh = std::make_unique<Mesh>();
        m_runwayMesh->initTextured(verts);
        std::cout << "[runways] loaded " << (verts.size() / 8 / 6) << " runway quads from " << runwaysPath << "\n";
    } else {
        std::cout << "[runways] no runway mesh built from " << runwaysPath << "\n";
    }
}

bool TerrainRenderer::sampleRunway(float worldX, float worldZ, TerrainSample& outSample) const {
    if (!m_runwaysEnabled || m_runwayColliders.empty()) {
        return false;
    }
    for (const auto& runway : m_runwayColliders) {
        Vec3 delta(worldX - runway.center.x, 0.0f, worldZ - runway.center.z);
        float along = delta.x * runway.dir.x + delta.z * runway.dir.z;
        float side = delta.x * runway.perp.x + delta.z * runway.perp.z;
        if (std::abs(along) > runway.halfLength || std::abs(side) > runway.halfWidth) {
            continue;
        }
        float t = (along + runway.halfLength) / (2.0f * runway.halfLength);
        t = std::clamp(t, 0.0f, 1.0f);
        float runwayY = runway.h0 + (runway.h1 - runway.h0) * t;

        float slopeY = (runway.h1 - runway.h0) / std::max(0.001f, runway.halfLength * 2.0f);
        Vec3 dirSlope(runway.dir.x, slopeY, runway.dir.z);
        Vec3 perpFlat(runway.perp.x, 0.0f, runway.perp.z);
        Vec3 normal = perpFlat.cross(dirSlope);
        if (normal.length() < 1e-4f) {
            normal = Vec3(0.0f, 1.0f, 0.0f);
        } else {
            normal = normal.normalized();
        }

        outSample.height = runwayY + m_runwayHeightOffset;
        outSample.normal = normal;
        outSample.water = 0.0f;
        outSample.urban = 0.0f;
        outSample.forest = 0.0f;
        outSample.onRunway = true;
        return true;
    }
    return false;
}

} // namespace nuage
