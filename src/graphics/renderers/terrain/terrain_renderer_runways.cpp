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
    m_runwayLayers.clear();

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
    bool markingsEnabled = false;
    const nlohmann::json* markingsConfig = nullptr;
    if (runwaysConfig.contains("markings") && runwaysConfig["markings"].is_object()) {
        markingsConfig = &runwaysConfig["markings"];
        markingsEnabled = markingsConfig->value("enabled", true);
    }

    std::filesystem::path cfg(configPath);
    auto resolve = [&](const std::string& p) {
        if (p.empty()) return p;
        std::filesystem::path path(p);
        if (path.is_absolute()) return p;
        return (cfg.parent_path() / path).string();
    };

    auto loadTexture = [&](const std::string& name, const std::string& path, bool repeat) -> Texture* {
        if (path.empty() || !m_assets) {
            return nullptr;
        }
        if (m_assets->loadTexture(name, path, repeat)) {
            return m_assets->getTexture(name);
        }
        return nullptr;
    };

    std::string runwayTexPath = resolve(runwaysConfig.value("texture", ""));
    if (!runwayTexPath.empty() && m_assets) {
        m_runwayTexture = loadTexture("runway_tex_base", runwayTexPath, true);
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

    Texture* centerlineTex = nullptr;
    Texture* thresholdTex = nullptr;
    Texture* aimTex = nullptr;
    float centerlineWidthMeters = 1.0f;
    float centerlineRepeatMeters = 20.0f;
    float centerlineInsetMeters = 30.0f;
    float thresholdDepthMeters = 12.0f;
    float thresholdInsetMeters = 0.0f;
    float edgeInsetMeters = 2.0f;
    float aimDepthMeters = 12.0f;
    float aimWidthMeters = 0.0f;
    float aimOffsetMeters = 300.0f;
    float markingsHeightOffset = m_runwayHeightOffset + 0.02f;
    if (markingsEnabled && markingsConfig) {
        centerlineTex = loadTexture("runway_tex_centerline",
                                    resolve(markingsConfig->value("centerlineTexture",
                                                                  std::string("../terrain/core/Runway/pa_centerline.png"))),
                                    true);
        thresholdTex = loadTexture("runway_tex_threshold",
                                   resolve(markingsConfig->value("thresholdTexture",
                                                                 std::string("../terrain/core/Runway/pa_threshold.png"))),
                                   false);
        aimTex = loadTexture("runway_tex_aim",
                             resolve(markingsConfig->value("aimTexture",
                                                           std::string("../terrain/core/Runway/pa_aim.png"))),
                             false);
        centerlineWidthMeters = std::max(0.1f, markingsConfig->value("centerlineWidthMeters", centerlineWidthMeters));
        centerlineRepeatMeters = std::max(0.1f, markingsConfig->value("centerlineRepeatMeters", centerlineRepeatMeters));
        centerlineInsetMeters = std::max(0.0f, markingsConfig->value("centerlineInsetMeters", centerlineInsetMeters));
        thresholdDepthMeters = std::max(0.1f, markingsConfig->value("thresholdDepthMeters", thresholdDepthMeters));
        thresholdInsetMeters = std::max(0.0f, markingsConfig->value("thresholdInsetMeters", thresholdInsetMeters));
        edgeInsetMeters = std::max(0.0f, markingsConfig->value("edgeInsetMeters", edgeInsetMeters));
        aimDepthMeters = std::max(0.1f, markingsConfig->value("aimDepthMeters", aimDepthMeters));
        aimWidthMeters = std::max(0.0f, markingsConfig->value("aimWidthMeters", aimWidthMeters));
        aimOffsetMeters = std::max(0.0f, markingsConfig->value("aimOffsetMeters", aimOffsetMeters));
        markingsHeightOffset = m_runwayHeightOffset
            + std::max(0.0f, markingsConfig->value("heightOffset", 0.02f));
    }

    std::vector<float> verts;
    std::vector<float> centerlineVerts;
    std::vector<float> thresholdVerts;
    std::vector<float> aimVerts;
    auto push = [&](std::vector<float>& buffer, const Vec3& pos, const Vec2& uv) {
        buffer.insert(buffer.end(), {
            pos.x, pos.y, pos.z,
            0.0f, 1.0f, 0.0f,
            uv.x, uv.y
        });
    };
    auto pushQuad = [&](std::vector<float>& buffer,
                        const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3,
                        const Vec2& uv0, const Vec2& uv1, const Vec2& uv2, const Vec2& uv3) {
        push(buffer, p0, uv0);
        push(buffer, p1, uv1);
        push(buffer, p2, uv2);

        push(buffer, p0, uv0);
        push(buffer, p2, uv2);
        push(buffer, p3, uv3);
    };
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

            push(verts, p0, Vec2(uvU0, uvV0));
            push(verts, p1, Vec2(uvU1, uvV0));
            push(verts, p2, Vec2(uvU1, uvV1));

            push(verts, p0, Vec2(uvU0, uvV0));
            push(verts, p2, Vec2(uvU1, uvV1));
            push(verts, p3, Vec2(uvU0, uvV1));

            if (markingsEnabled) {
                auto lerpY = [&](float t) {
                    return lePos.y + (hePos.y - lePos.y) * t;
                };
                float safeInset = std::min(centerlineInsetMeters, length * 0.4f);
                float centerlineLength = length - 2.0f * safeInset;
                if (centerlineTex && centerlineLength > 1.0f) {
                    Vec3 start = lePos + dir * safeInset;
                    Vec3 end = hePos - dir * safeInset;
                    start.y = lerpY(safeInset / length) + markingsHeightOffset;
                    end.y = lerpY(1.0f - safeInset / length) + markingsHeightOffset;
                    float halfCenterline = centerlineWidthMeters * 0.5f;
                    Vec3 clOffset = perp * halfCenterline;
                    Vec3 c0(start.x + clOffset.x, start.y, start.z + clOffset.z);
                    Vec3 c1(start.x - clOffset.x, start.y, start.z - clOffset.z);
                    Vec3 c2(end.x - clOffset.x, end.y, end.z - clOffset.z);
                    Vec3 c3(end.x + clOffset.x, end.y, end.z + clOffset.z);
                    float uvV = centerlineLength / centerlineRepeatMeters;
                    pushQuad(centerlineVerts, c0, c1, c2, c3,
                             Vec2(0.0f, 0.0f), Vec2(1.0f, 0.0f),
                             Vec2(1.0f, uvV), Vec2(0.0f, uvV));
                }

                float thresholdWidth = std::max(1.0f, widthMeters - 2.0f * edgeInsetMeters);
                float thresholdHalfWidth = thresholdWidth * 0.5f;
                float thresholdEnd = thresholdInsetMeters + thresholdDepthMeters;
                if (thresholdTex && thresholdEnd < length * 0.45f) {
                    auto addThreshold = [&](const Vec3& base, float t0, float t1) {
                        Vec3 a = base + dir * t0;
                        Vec3 b = base + dir * t1;
                        a.y = lerpY(t0 / length) + markingsHeightOffset;
                        b.y = lerpY(t1 / length) + markingsHeightOffset;
                        Vec3 offset = perp * thresholdHalfWidth;
                        Vec3 q0(a.x + offset.x, a.y, a.z + offset.z);
                        Vec3 q1(a.x - offset.x, a.y, a.z - offset.z);
                        Vec3 q2(b.x - offset.x, b.y, b.z - offset.z);
                        Vec3 q3(b.x + offset.x, b.y, b.z + offset.z);
                        pushQuad(thresholdVerts, q0, q1, q2, q3,
                                 Vec2(0.0f, 0.0f), Vec2(1.0f, 0.0f),
                                 Vec2(1.0f, 1.0f), Vec2(0.0f, 1.0f));
                    };
                    addThreshold(lePos, thresholdInsetMeters, thresholdEnd);
                    float t1 = length - thresholdInsetMeters;
                    float t0 = length - thresholdEnd;
                    addThreshold(lePos, t0, t1);
                }

                float aimWidth = aimWidthMeters;
                if (aimWidth <= 0.1f) {
                    aimWidth = std::max(6.0f, widthMeters * 0.2f);
                }
                aimWidth = std::min(aimWidth, widthMeters - 2.0f * edgeInsetMeters);
                aimWidth = std::max(0.1f, aimWidth);
                float aimHalfWidth = aimWidth * 0.5f;
                float aimHalfDepth = aimDepthMeters * 0.5f;
                if (aimTex && aimOffsetMeters + aimHalfDepth < length * 0.5f) {
                    auto addAim = [&](float centerDist) {
                        float t0 = centerDist - aimHalfDepth;
                        float t1 = centerDist + aimHalfDepth;
                        Vec3 a = lePos + dir * t0;
                        Vec3 b = lePos + dir * t1;
                        a.y = lerpY(t0 / length) + markingsHeightOffset;
                        b.y = lerpY(t1 / length) + markingsHeightOffset;
                        Vec3 offset = perp * aimHalfWidth;
                        Vec3 q0(a.x + offset.x, a.y, a.z + offset.z);
                        Vec3 q1(a.x - offset.x, a.y, a.z - offset.z);
                        Vec3 q2(b.x - offset.x, b.y, b.z - offset.z);
                        Vec3 q3(b.x + offset.x, b.y, b.z + offset.z);
                        pushQuad(aimVerts, q0, q1, q2, q3,
                                 Vec2(0.0f, 0.0f), Vec2(1.0f, 0.0f),
                                 Vec2(1.0f, 1.0f), Vec2(0.0f, 1.0f));
                    };
                    if (aimOffsetMeters + aimHalfDepth < length) {
                        addAim(aimOffsetMeters);
                    }
                    if (length - aimOffsetMeters - aimHalfDepth > 0.0f) {
                        addAim(length - aimOffsetMeters);
                    }
                }
            }
        }
    }

    if (!verts.empty()) {
        m_runwayMesh = std::make_unique<Mesh>();
        m_runwayMesh->initTextured(verts);
        std::cout << "[runways] loaded " << (verts.size() / 8 / 6) << " runway quads from " << runwaysPath << "\n";
    } else {
        std::cout << "[runways] no runway mesh built from " << runwaysPath << "\n";
    }
    auto addLayer = [&](std::vector<float>& data, Texture* texture, const char* label) {
        if (data.empty() || !texture) {
            return;
        }
        RunwayLayer layer;
        layer.mesh = std::make_unique<Mesh>();
        layer.mesh->initTextured(data);
        layer.texture = texture;
        m_runwayLayers.push_back(std::move(layer));
        if (m_compiledDebugLog) {
            std::cout << "[runways] built " << label << " mesh\n";
        }
    };
    addLayer(centerlineVerts, centerlineTex, "centerline");
    addLayer(thresholdVerts, thresholdTex, "threshold");
    addLayer(aimVerts, aimTex, "aim");
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
