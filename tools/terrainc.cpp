#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "utils/stb_image.h"
#include "utils/config_loader.hpp"
#include "utils/json.hpp"
#include "math/vec2.hpp"
#include "math/vec3.hpp"
#include "tools/terrainc/color_ramp.hpp"
#include "tools/terrainc/heightmap.hpp"
#include "tools/terrainc/mask_smoothing.hpp"

namespace {
struct Config {
    std::string heightmapPath;
    std::string osmPath;
    std::string outDir;
    std::string runwaysJsonPath;
    float sizeX = 0.0f;
    float sizeZ = 0.0f;
    float heightMin = 0.0f;
    float heightMax = 1000.0f;
    float tileSize = 2000.0f;
    int gridResolution = 129;
    int maskResolution = 0;
    int maskSmooth = 0;
    double xmin = 0.0;
    double ymin = 0.0;
    double xmax = 0.0;
    double ymax = 0.0;
    bool hasBbox = false;
    double originLat = 0.0;
    double originLon = 0.0;
    double originAlt = 0.0;
    float runwayBlendMeters = 60.0f;
};

struct RunwayInput {
    nuage::Vec3 le;
    nuage::Vec3 he;
    float widthMeters = 0.0f;
    std::string airportIdent;
    std::string leIdent;
    std::string heIdent;
    std::string closed;
    double widthFt = 0.0;
};

struct Runway {
    nuage::Vec3 center;
    nuage::Vec3 dir;
    nuage::Vec3 perp;
    float halfLength = 0.0f;
    float halfWidth = 0.0f;
    float h0 = 0.0f;
    float h1 = 0.0f;
};

void printUsage() {
    std::cout << "Usage: terrainc --heightmap <path> --size-x <meters> --size-z <meters>\n"
              << "               --height-min <m> --height-max <m> --tile-size <m>\n"
              << "               --grid <cells> --out <dir>\n"
              << "               [--runways-json <path> --runway-blend <meters>]\n"
              << "               [--origin-lat <deg>] [--origin-lon <deg>] [--origin-alt <m>]\n"
              << "               [--osm <path> --mask-res <pixels> --xmin <lon> --ymin <lat>\n"
              << "                --xmax <lon> --ymax <lat> --mask-smooth <passes>]\n";
}

bool parseArgs(int argc, char** argv, Config& cfg) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](std::string& out) -> bool {
            if (i + 1 >= argc) return false;
            out = argv[++i];
            return true;
        };

        if (arg == "--heightmap") {
            if (!next(cfg.heightmapPath)) return false;
        } else if (arg == "--size-x") {
            std::string v;
            if (!next(v)) return false;
            cfg.sizeX = std::stof(v);
        } else if (arg == "--size-z") {
            std::string v;
            if (!next(v)) return false;
            cfg.sizeZ = std::stof(v);
        } else if (arg == "--height-min") {
            std::string v;
            if (!next(v)) return false;
            cfg.heightMin = std::stof(v);
        } else if (arg == "--height-max") {
            std::string v;
            if (!next(v)) return false;
            cfg.heightMax = std::stof(v);
        } else if (arg == "--tile-size") {
            std::string v;
            if (!next(v)) return false;
            cfg.tileSize = std::stof(v);
        } else if (arg == "--grid") {
            std::string v;
            if (!next(v)) return false;
            cfg.gridResolution = std::stoi(v);
        } else if (arg == "--out") {
            if (!next(cfg.outDir)) return false;
        } else if (arg == "--runways-json") {
            if (!next(cfg.runwaysJsonPath)) return false;
        } else if (arg == "--runway-blend") {
            std::string v;
            if (!next(v)) return false;
            cfg.runwayBlendMeters = std::stof(v);
        } else if (arg == "--osm") {
            if (!next(cfg.osmPath)) return false;
        } else if (arg == "--mask-res") {
            std::string v;
            if (!next(v)) return false;
            cfg.maskResolution = std::stoi(v);
        } else if (arg == "--mask-smooth") {
            std::string v;
            if (!next(v)) return false;
            cfg.maskSmooth = std::stoi(v);
        } else if (arg == "--xmin") {
            std::string v;
            if (!next(v)) return false;
            cfg.xmin = std::stod(v);
            cfg.hasBbox = true;
        } else if (arg == "--ymin") {
            std::string v;
            if (!next(v)) return false;
            cfg.ymin = std::stod(v);
            cfg.hasBbox = true;
        } else if (arg == "--xmax") {
            std::string v;
            if (!next(v)) return false;
            cfg.xmax = std::stod(v);
            cfg.hasBbox = true;
        } else if (arg == "--ymax") {
            std::string v;
            if (!next(v)) return false;
            cfg.ymax = std::stod(v);
            cfg.hasBbox = true;
        } else if (arg == "--origin-lat") {
            std::string v;
            if (!next(v)) return false;
            cfg.originLat = std::stod(v);
        } else if (arg == "--origin-lon") {
            std::string v;
            if (!next(v)) return false;
            cfg.originLon = std::stod(v);
        } else if (arg == "--origin-alt") {
            std::string v;
            if (!next(v)) return false;
            cfg.originAlt = std::stod(v);
        } else {
            std::cerr << "Unknown arg: " << arg << "\n";
            return false;
        }
    }

    if (cfg.heightmapPath.empty() || cfg.outDir.empty()) {
        return false;
    }
    if (cfg.tileSize <= 0.0f) cfg.tileSize = 2000.0f;
    cfg.gridResolution = std::max(2, cfg.gridResolution);
    if (cfg.heightMax <= cfg.heightMin) cfg.heightMax = cfg.heightMin + 1.0f;
    cfg.runwayBlendMeters = std::max(0.0f, cfg.runwayBlendMeters);
    return true;
}

bool writeMesh(const std::filesystem::path& path, const std::vector<float>& verts) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    const char magic[4] = {'N', 'T', 'M', '1'};
    std::uint32_t count = static_cast<std::uint32_t>(verts.size());
    out.write(magic, 4);
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    out.write(reinterpret_cast<const char*>(verts.data()), static_cast<std::streamsize>(verts.size() * sizeof(float)));
    return static_cast<bool>(out);
}

void writeTileMeta(const std::filesystem::path& path, int tx, int ty, float minH, float maxH, int grid) {
    std::ofstream out(path);
    out << "{\n";
    out << "  \"tileId\": [" << tx << ", " << ty << "],\n";
    out << "  \"gridResolution\": " << grid << ",\n";
    out << "  \"minHeight\": " << minH << ",\n";
    out << "  \"maxHeight\": " << maxH << "\n";
    out << "}\n";
}

struct Polygon {
    std::vector<nuage::Vec2> ring;
    float minX = 0.0f;
    float minZ = 0.0f;
    float maxX = 0.0f;
    float maxZ = 0.0f;
    int classId = 0;
};

bool pointInPolygon(const std::vector<nuage::Vec2>& poly, float x, float z) {
    bool inside = false;
    size_t count = poly.size();
    for (size_t i = 0, j = count - 1; i < count; j = i++) {
        const auto& a = poly[i];
        const auto& b = poly[j];
        bool intersect = ((a.y > z) != (b.y > z)) &&
            (x < (b.x - a.x) * (z - a.y) / (b.y - a.y + 1e-9f) + a.x);
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
}

int classFromTags(const nlohmann::json& tags) {
    auto getTag = [&](const char* key) -> std::string {
        if (!tags.contains(key)) {
            return {};
        }
        const auto& val = tags[key];
        if (!val.is_string()) {
            return {};
        }
        return val.get<std::string>();
    };

    std::string landuse = getTag("landuse");
    if (!landuse.empty()) {
        std::string v = landuse;
        if (v == "residential" || v == "commercial" || v == "industrial" || v == "retail") {
            return 2;
        }
        if (v == "forest" || v == "wood") {
            return 3;
        }
        if (v == "meadow" || v == "grass" || v == "grassland" || v == "farmland" ||
            v == "orchard" || v == "vineyard" || v == "plant_nursery" || v == "greenfield") {
            return 4;
        }
        return 4;
    }
    std::string natural = getTag("natural");
    if (!natural.empty()) {
        std::string v = natural;
        if (v == "wood" || v == "forest") {
            return 3;
        }
        if (v == "grassland" || v == "scrub" || v == "heath") {
            return 4;
        }
    }
    return 0;
}

struct Projection {
    double lon0 = 0.0;
    double lat0 = 0.0;
    double metersPerLon = 0.0;
    double metersPerLat = 0.0;
};

Projection makeProjection(double xmin, double ymin, double xmax, double ymax) {
    Projection proj;
    proj.lon0 = (xmin + xmax) * 0.5;
    proj.lat0 = (ymin + ymax) * 0.5;
    double latRad = proj.lat0 * 3.141592653589793 / 180.0;
    proj.metersPerLat = 111320.0;
    proj.metersPerLon = 111320.0 * std::cos(latRad);
    return proj;
}

nuage::Vec2 projectLonLat(const Projection& proj, double lon, double lat) {
    float x = static_cast<float>((lon - proj.lon0) * proj.metersPerLon);
    float z = static_cast<float>((lat - proj.lat0) * proj.metersPerLat);
    return nuage::Vec2(x, z);
}

float sampleHeightAtWorld(const Heightmap& hm, const Config& cfg, float minX, float minZ,
                          float heightRange, float worldX, float worldZ) {
    float u = (worldX - minX) / cfg.sizeX;
    float v = (worldZ - minZ) / cfg.sizeZ;
    u = clamp01(u);
    v = clamp01(v);
    float hx = u * static_cast<float>(hm.width - 1);
    float hz = v * static_cast<float>(hm.height - 1);
    float raw = bilinearSample(hm, hx, hz) / 65535.0f;
    return cfg.heightMin + raw * heightRange;
}

float applyRunwayFlatten(float worldX, float worldZ, float baseHeight,
                         const std::vector<Runway>& runways, float blendMeters) {
    if (runways.empty()) {
        return baseHeight;
    }

    float bestWeight = 0.0f;
    float bestHeight = baseHeight;
    float blend = std::max(blendMeters, 0.001f);

    for (const auto& runway : runways) {
        nuage::Vec3 delta(worldX - runway.center.x, 0.0f, worldZ - runway.center.z);
        float along = delta.x * runway.dir.x + delta.z * runway.dir.z;
        float side = delta.x * runway.perp.x + delta.z * runway.perp.z;
        float absAlong = std::abs(along);
        float absSide = std::abs(side);

        if (absAlong > runway.halfLength + blend || absSide > runway.halfWidth + blend) {
            continue;
        }

        float dx = std::max(absAlong - runway.halfLength, 0.0f);
        float dz = std::max(absSide - runway.halfWidth, 0.0f);
        float dist = std::sqrt(dx * dx + dz * dz);
        float weight = 1.0f;
        if (dist > 0.0f) {
            weight = 1.0f - std::min(dist / blend, 1.0f);
        }

        float t = (along + runway.halfLength) / (2.0f * runway.halfLength);
        t = std::clamp(t, 0.0f, 1.0f);
        float runwayHeight = runway.h0 + (runway.h1 - runway.h0) * t;

        if (weight > bestWeight) {
            bestWeight = weight;
            bestHeight = runwayHeight;
        }
    }

    if (bestWeight <= 0.0f) {
        return baseHeight;
    }
    return baseHeight + (bestHeight - baseHeight) * bestWeight;
}

bool parseDoubleSafe(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtod(s.c_str(), &end);
    return end != s.c_str();
}

bool runCommand(const std::string& command) {
    int result = std::system(command.c_str());
    return result == 0;
}

std::vector<Polygon> loadPolygonsFromGeoJson(const std::string& path, int defaultClass,
                                             const Projection& proj, bool useTags) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Failed to open GeoJSON: " << path << "\n";
        return {};
    }
    nlohmann::json doc;
    in >> doc;
    if (!doc.contains("features") || !doc["features"].is_array()) {
        return {};
    }

    std::vector<Polygon> polys;
    for (const auto& feature : doc["features"]) {
        if (!feature.contains("geometry")) continue;
        const auto& geom = feature["geometry"];
        if (!geom.contains("type") || !geom.contains("coordinates")) continue;

        int classId = defaultClass;
        if (useTags && feature.contains("properties")) {
            const auto& props = feature["properties"];
            if (props.contains("tags") && props["tags"].is_object()) {
                classId = classFromTags(props["tags"]);
            } else {
                classId = classFromTags(props);
            }
        }
        if (classId == 0) {
            continue;
        }

        std::string type = geom["type"].get<std::string>();
        const auto& coords = geom["coordinates"];
        auto addRing = [&](const nlohmann::json& ring) {
            if (!ring.is_array() || ring.size() < 3) return;
            Polygon poly;
            poly.classId = classId;
            poly.minX = poly.minZ = std::numeric_limits<float>::max();
            poly.maxX = poly.maxZ = std::numeric_limits<float>::lowest();
            for (const auto& pt : ring) {
                if (!pt.is_array() || pt.size() < 2) continue;
                double lon = pt[0].get<double>();
                double lat = pt[1].get<double>();
                nuage::Vec2 p = projectLonLat(proj, lon, lat);
                poly.minX = std::min(poly.minX, p.x);
                poly.maxX = std::max(poly.maxX, p.x);
                poly.minZ = std::min(poly.minZ, p.y);
                poly.maxZ = std::max(poly.maxZ, p.y);
                poly.ring.push_back(p);
            }
            if (poly.ring.size() >= 3) {
                polys.push_back(std::move(poly));
            }
        };

        if (type == "Polygon") {
            if (coords.is_array() && !coords.empty()) {
                addRing(coords[0]);
            }
        } else if (type == "MultiPolygon") {
            if (coords.is_array()) {
                for (const auto& poly : coords) {
                    if (poly.is_array() && !poly.empty()) {
                        addRing(poly[0]);
                    }
                }
            }
        }
    }
    return polys;
}

void rasterizePolygonsToMask(std::vector<std::uint8_t>& mask, int maskRes,
                             float tileMinX, float tileMinZ, float tileSize,
                             const std::vector<Polygon>& polys, int classPriority) {
    if (maskRes <= 0) return;
    for (const auto& poly : polys) {
        if (poly.classId != classPriority) continue;
        if (poly.maxX <= tileMinX || poly.minX >= tileMinX + tileSize ||
            poly.maxZ <= tileMinZ || poly.minZ >= tileMinZ + tileSize) {
            continue;
        }
        int x0 = static_cast<int>(std::floor((poly.minX - tileMinX) / tileSize * maskRes));
        int x1 = static_cast<int>(std::ceil((poly.maxX - tileMinX) / tileSize * maskRes));
        int z0 = static_cast<int>(std::floor((poly.minZ - tileMinZ) / tileSize * maskRes));
        int z1 = static_cast<int>(std::ceil((poly.maxZ - tileMinZ) / tileSize * maskRes));
        x0 = std::clamp(x0, 0, maskRes - 1);
        x1 = std::clamp(x1, 0, maskRes - 1);
        z0 = std::clamp(z0, 0, maskRes - 1);
        z1 = std::clamp(z1, 0, maskRes - 1);

        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                float fx = (static_cast<float>(x) + 0.5f) / maskRes;
                float fz = (static_cast<float>(z) + 0.5f) / maskRes;
                float worldX = tileMinX + fx * tileSize;
                float worldZ = tileMinZ + fz * tileSize;
                if (pointInPolygon(poly.ring, worldX, worldZ)) {
                    std::uint8_t& cell = mask[z * maskRes + x];
                    if (classPriority == 1) {
                        cell = 1;
                    } else if (cell == 0) {
                        cell = static_cast<std::uint8_t>(classPriority);
                    }
                }
            }
        }
    }
}

bool writeMask(const std::filesystem::path& path, const std::vector<std::uint8_t>& mask) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(reinterpret_cast<const char*>(mask.data()), static_cast<std::streamsize>(mask.size()));
    return static_cast<bool>(out);
}

std::int64_t tileKey(int x, int y) {
    return (static_cast<std::int64_t>(x) << 32) ^ static_cast<std::uint32_t>(y);
}

void rasterizePolygonsToMaskList(std::vector<std::uint8_t>& mask, int maskRes,
                                 float tileMinX, float tileMinZ, float tileSize,
                                 const std::vector<Polygon>& polys,
                                 const std::vector<int>& indices) {
    if (maskRes <= 0 || indices.empty()) return;
    for (int idx : indices) {
        const auto& poly = polys[static_cast<size_t>(idx)];
        if (poly.maxX <= tileMinX || poly.minX >= tileMinX + tileSize ||
            poly.maxZ <= tileMinZ || poly.minZ >= tileMinZ + tileSize) {
            continue;
        }
        int x0 = static_cast<int>(std::floor((poly.minX - tileMinX) / tileSize * maskRes));
        int x1 = static_cast<int>(std::ceil((poly.maxX - tileMinX) / tileSize * maskRes));
        int z0 = static_cast<int>(std::floor((poly.minZ - tileMinZ) / tileSize * maskRes));
        int z1 = static_cast<int>(std::ceil((poly.maxZ - tileMinZ) / tileSize * maskRes));
        x0 = std::clamp(x0, 0, maskRes - 1);
        x1 = std::clamp(x1, 0, maskRes - 1);
        z0 = std::clamp(z0, 0, maskRes - 1);
        z1 = std::clamp(z1, 0, maskRes - 1);

        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                float fx = (static_cast<float>(x) + 0.5f) / maskRes;
                float fz = (static_cast<float>(z) + 0.5f) / maskRes;
                float worldX = tileMinX + fx * tileSize;
                float worldZ = tileMinZ + fz * tileSize;
                if (pointInPolygon(poly.ring, worldX, worldZ)) {
                    std::uint8_t& cell = mask[z * maskRes + x];
                    if (poly.classId == 1) {
                        cell = 1;
                    } else if (cell == 0) {
                        cell = static_cast<std::uint8_t>(poly.classId);
                    }
                }
            }
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    Config cfg;
    if (!parseArgs(argc, argv, cfg)) {
        printUsage();
        return 1;
    }

    if (cfg.maskResolution > 0 && cfg.osmPath.empty()) {
        std::cerr << "Mask resolution set but no OSM file provided.\n";
        return 1;
    }
    if (!cfg.osmPath.empty() && !cfg.hasBbox) {
        std::cerr << "OSM provided but bbox missing; use --xmin/--ymin/--xmax/--ymax.\n";
        return 1;
    }

    Projection proj;
    if (cfg.hasBbox) {
        proj = makeProjection(cfg.xmin, cfg.ymin, cfg.xmax, cfg.ymax);
        double widthMeters = (cfg.xmax - cfg.xmin) * proj.metersPerLon;
        double heightMeters = (cfg.ymax - cfg.ymin) * proj.metersPerLat;
        if (cfg.sizeX <= 0.0f) cfg.sizeX = static_cast<float>(std::abs(widthMeters));
        if (cfg.sizeZ <= 0.0f) cfg.sizeZ = static_cast<float>(std::abs(heightMeters));
        cfg.originLat = proj.lat0;
        cfg.originLon = proj.lon0;
    }

    if (cfg.sizeX <= 0.0f || cfg.sizeZ <= 0.0f) {
        std::cerr << "Size must be set via --size-x/--size-z or bbox.\n";
        return 1;
    }

    Heightmap hm;
    if (!loadHeightmap(cfg.heightmapPath, hm)) {
        return 1;
    }

    std::filesystem::path outDir(cfg.outDir);
    std::filesystem::path tilesDir = outDir / "tiles";
    std::filesystem::create_directories(tilesDir);

    float halfX = cfg.sizeX * 0.5f;
    float halfZ = cfg.sizeZ * 0.5f;
    float minX = -halfX;
    float minZ = -halfZ;
    float maxX = halfX;
    float maxZ = halfZ;

    int minTileX = static_cast<int>(std::floor(minX / cfg.tileSize));
    int maxTileX = static_cast<int>(std::ceil(maxX / cfg.tileSize)) - 1;
    int minTileZ = static_cast<int>(std::floor(minZ / cfg.tileSize));
    int maxTileZ = static_cast<int>(std::ceil(maxZ / cfg.tileSize)) - 1;

    std::vector<std::pair<int, int>> tileIndex;
    tileIndex.reserve(static_cast<size_t>((maxTileX - minTileX + 1) * (maxTileZ - minTileZ + 1)));

    float heightRange = cfg.heightMax - cfg.heightMin;

    std::vector<RunwayInput> runwayInputs;
    std::vector<Runway> runways;
    std::vector<RunwayInput> runwayOutputs;
    if (!cfg.runwaysJsonPath.empty()) {
        auto runwaysOpt = nuage::loadJsonConfig(cfg.runwaysJsonPath);
        if (!runwaysOpt) {
            std::cerr << "Failed to load runways JSON: " << cfg.runwaysJsonPath << "\n";
            return 1;
        }
        const auto& runwaysJson = *runwaysOpt;
        std::unordered_set<std::string> heliports;
        if (runwaysJson.contains("airports") && runwaysJson["airports"].is_array()) {
            for (const auto& airport : runwaysJson["airports"]) {
                if (!airport.is_object()) continue;
                std::string type = airport.value("type", "");
                if (type != "heliport") continue;
                std::string ident = airport.value("ident", "");
                if (!ident.empty()) {
                    heliports.insert(ident);
                }
            }
        }

        if (runwaysJson.contains("runways") && runwaysJson["runways"].is_array()) {
            for (const auto& runway : runwaysJson["runways"]) {
                if (!runway.is_object()) continue;
                std::string airportIdent = runway.value("airportIdent", "");
                if (!airportIdent.empty() && heliports.find(airportIdent) != heliports.end()) {
                    continue;
                }
                std::string closed = runway.value("closed", "");
                if (closed == "1" || closed == "true" || closed == "TRUE") {
                    continue;
                }

                if (!runway.contains("leENU") || !runway.contains("heENU")) continue;
                const auto& le = runway["leENU"];
                const auto& he = runway["heENU"];
                if (!le.is_array() || !he.is_array() || le.size() != 3 || he.size() != 3) {
                    continue;
                }

                std::string widthFtStr = runway.value("widthFt", "");
                double widthFt = 0.0;
                if (!parseDoubleSafe(widthFtStr, widthFt)) {
                    continue;
                }
                float widthMeters = static_cast<float>(widthFt * 0.3048);
                if (widthMeters <= 0.0f) {
                    continue;
                }

                RunwayInput input;
                input.le = nuage::Vec3(le[0].get<float>(), le[1].get<float>(), le[2].get<float>());
                input.he = nuage::Vec3(he[0].get<float>(), he[1].get<float>(), he[2].get<float>());
                input.widthMeters = widthMeters;
                input.airportIdent = airportIdent;
                input.leIdent = runway.value("leIdent", "");
                input.heIdent = runway.value("heIdent", "");
                input.closed = closed;
                input.widthFt = widthFt;
                runwayInputs.push_back(input);
            }
        }

        for (const auto& input : runwayInputs) {
            if (input.le.x < minX || input.le.x > maxX || input.le.z < minZ || input.le.z > maxZ) {
                continue;
            }
            if (input.he.x < minX || input.he.x > maxX || input.he.z < minZ || input.he.z > maxZ) {
                continue;
            }
            float dx = input.he.x - input.le.x;
            float dz = input.he.z - input.le.z;
            float length = std::sqrt(dx * dx + dz * dz);
            if (length < 1.0f) {
                continue;
            }

            Runway runway;
            runway.center = (input.le + input.he) * 0.5f;
            runway.dir = nuage::Vec3(dx / length, 0.0f, dz / length);
            runway.perp = nuage::Vec3(-runway.dir.z, 0.0f, runway.dir.x);
            runway.halfLength = length * 0.5f;
            runway.halfWidth = input.widthMeters * 0.5f;
            runway.h0 = sampleHeightAtWorld(hm, cfg, minX, minZ, heightRange, input.le.x, input.le.z);
            runway.h1 = sampleHeightAtWorld(hm, cfg, minX, minZ, heightRange, input.he.x, input.he.z);
            runways.push_back(runway);

            RunwayInput output = input;
            output.le.y = runway.h0;
            output.he.y = runway.h1;
            runwayOutputs.push_back(output);
        }
        if (!runwayInputs.empty()) {
            std::cout << "[terrainc] runways loaded: " << runways.size() << "\n";
        }

        if (!runwayOutputs.empty()) {
            std::filesystem::path outRunwaysPath = outDir / "runways.json";
            std::ofstream runwaysOut(outRunwaysPath);
            if (runwaysOut.is_open()) {
                nlohmann::json outJson;
                outJson["source"] = "terrainc";
                outJson["runways"] = nlohmann::json::array();
                for (const auto& output : runwayOutputs) {
                    nlohmann::json entry;
                    entry["airportIdent"] = output.airportIdent;
                    entry["leIdent"] = output.leIdent;
                    entry["heIdent"] = output.heIdent;
                    entry["closed"] = output.closed;
                    entry["widthFt"] = output.widthFt;
                    entry["leENU"] = {output.le.x, output.le.y, output.le.z};
                    entry["heENU"] = {output.he.x, output.he.y, output.he.z};
                    outJson["runways"].push_back(entry);
                }
                runwaysOut << outJson.dump(2) << "\n";
                std::cout << "[terrainc] wrote runways runtime: " << outRunwaysPath.string() << "\n";
            } else {
                std::cerr << "Failed to write runways runtime JSON.\n";
            }
        }
    }

    std::vector<Polygon> waterPolys;
    std::vector<Polygon> landPolys;
    std::vector<Polygon> allPolys;
    std::unordered_map<std::int64_t, std::vector<int>> polyBuckets;
    if (!cfg.osmPath.empty()) {
        std::filesystem::path outDir(cfg.outDir);
        std::filesystem::path tmpDir = outDir / "osm_tmp";
        std::filesystem::create_directories(tmpDir);

        std::filesystem::path areaPbf = tmpDir / "area.pbf";
        std::filesystem::path waterPbf = tmpDir / "water.pbf";
        std::filesystem::path landPbf = tmpDir / "landuse.pbf";
        std::filesystem::path waterGeo = tmpDir / "water.geojson";
        std::filesystem::path landGeo = tmpDir / "landuse.geojson";

        std::ostringstream bbox;
        bbox << cfg.xmin << "," << cfg.ymin << "," << cfg.xmax << "," << cfg.ymax;

        std::ostringstream extractCmd;
        extractCmd << "osmium extract -b " << bbox.str()
                   << " \"" << cfg.osmPath << "\" -o \"" << areaPbf.string() << "\"";
        if (!runCommand(extractCmd.str())) {
            std::cerr << "Failed to extract OSM bbox.\n";
            return 1;
        }

        std::ostringstream waterCmd;
        waterCmd << "osmium tags-filter -o \"" << waterPbf.string() << "\" \"" << areaPbf.string()
                 << "\" w/natural=water w/waterway=riverbank w/water=* w/natural=wetland"
                 << " r/natural=water r/waterway=riverbank r/water=* r/natural=wetland";
        if (!runCommand(waterCmd.str())) {
            std::cerr << "Failed to filter water from OSM.\n";
            return 1;
        }

        std::ostringstream landCmd;
        landCmd << "osmium tags-filter -o \"" << landPbf.string() << "\" \"" << areaPbf.string()
                << "\" w/landuse=* w/natural=wood w/natural=grassland w/natural=heath w/natural=scrub"
                << " r/landuse=* r/natural=wood r/natural=grassland r/natural=heath r/natural=scrub";
        if (!runCommand(landCmd.str())) {
            std::cerr << "Failed to filter landuse from OSM.\n";
            return 1;
        }

        std::ostringstream waterExport;
        waterExport << "osmium export -f geojson -o \"" << waterGeo.string() << "\" \"" << waterPbf.string() << "\"";
        if (!runCommand(waterExport.str())) {
            std::cerr << "Failed to export water GeoJSON.\n";
            return 1;
        }

        std::ostringstream landExport;
        landExport << "osmium export -f geojson -o \"" << landGeo.string() << "\" \"" << landPbf.string() << "\"";
        if (!runCommand(landExport.str())) {
            std::cerr << "Failed to export landuse GeoJSON.\n";
            return 1;
        }

        landPolys = loadPolygonsFromGeoJson(landGeo.string(), 0, proj, true);
        waterPolys = loadPolygonsFromGeoJson(waterGeo.string(), 1, proj, false);
        std::cout << "Loaded landuse polygons: " << landPolys.size() << "\n";
        std::cout << "Loaded water polygons: " << waterPolys.size() << "\n";
    }

    if (!landPolys.empty() || !waterPolys.empty()) {
        allPolys.reserve(landPolys.size() + waterPolys.size());
        for (const auto& poly : landPolys) {
            allPolys.push_back(poly);
        }
        for (const auto& poly : waterPolys) {
            allPolys.push_back(poly);
        }

        for (size_t i = 0; i < allPolys.size(); ++i) {
            const auto& poly = allPolys[i];
            int minTx = static_cast<int>(std::floor(poly.minX / cfg.tileSize));
            int maxTx = static_cast<int>(std::floor(poly.maxX / cfg.tileSize));
            int minTz = static_cast<int>(std::floor(poly.minZ / cfg.tileSize));
            int maxTz = static_cast<int>(std::floor(poly.maxZ / cfg.tileSize));

            minTx = std::max(minTx, minTileX);
            maxTx = std::min(maxTx, maxTileX);
            minTz = std::max(minTz, minTileZ);
            maxTz = std::min(maxTz, maxTileZ);

            for (int ty = minTz; ty <= maxTz; ++ty) {
                for (int tx = minTx; tx <= maxTx; ++tx) {
                    polyBuckets[tileKey(tx, ty)].push_back(static_cast<int>(i));
                }
            }
        }
    }

    for (int ty = minTileZ; ty <= maxTileZ; ++ty) {
        for (int tx = minTileX; tx <= maxTileX; ++tx) {
            float tileMinX = static_cast<float>(tx) * cfg.tileSize;
            float tileMinZ = static_cast<float>(ty) * cfg.tileSize;
            float tileMaxX = tileMinX + cfg.tileSize;
            float tileMaxZ = tileMinZ + cfg.tileSize;

            if (tileMaxX <= minX || tileMinX >= maxX || tileMaxZ <= minZ || tileMinZ >= maxZ) {
                continue;
            }

            int cells = cfg.gridResolution;
            int resX = cells + 1;
            int resZ = cells + 1;

            std::vector<nuage::Vec3> positions(resX * resZ);
            std::vector<nuage::Vec3> normals(resX * resZ, nuage::Vec3(0, 1, 0));
            std::vector<float> verts;
            int stride = 9;
            verts.reserve((resX - 1) * (resZ - 1) * 6 * stride);

            float localMinH = std::numeric_limits<float>::max();
            float localMaxH = std::numeric_limits<float>::lowest();

            for (int z = 0; z < resZ; ++z) {
                for (int x = 0; x < resX; ++x) {
                    float fx = (resX > 1) ? static_cast<float>(x) / (resX - 1) : 0.0f;
                    float fz = (resZ > 1) ? static_cast<float>(z) / (resZ - 1) : 0.0f;

                    float worldX = tileMinX + fx * cfg.tileSize;
                    float worldZ = tileMinZ + fz * cfg.tileSize;

                    float u = (worldX - minX) / cfg.sizeX;
                    float v = (worldZ - minZ) / cfg.sizeZ;
                    u = clamp01(u);
                    v = clamp01(v);

                    float hx = u * static_cast<float>(hm.width - 1);
                    float hz = v * static_cast<float>(hm.height - 1);
                    float raw = bilinearSample(hm, hx, hz) / 65535.0f;
                    float height = cfg.heightMin + raw * heightRange;
                    height = applyRunwayFlatten(worldX, worldZ, height, runways, cfg.runwayBlendMeters);

                    localMinH = std::min(localMinH, height);
                    localMaxH = std::max(localMaxH, height);

                    int idx = z * resX + x;
                    positions[idx] = nuage::Vec3(worldX, height, worldZ);
                }
            }

            for (int z = 0; z < resZ; ++z) {
                for (int x = 0; x < resX; ++x) {
                    int idx = z * resX + x;
                    int left = z * resX + std::max(x - 1, 0);
                    int right = z * resX + std::min(x + 1, resX - 1);
                    int up = std::max(z - 1, 0) * resX + x;
                    int down = std::min(z + 1, resZ - 1) * resX + x;

                    nuage::Vec3 tangentX = positions[right] - positions[left];
                    nuage::Vec3 tangentZ = positions[down] - positions[up];
                    nuage::Vec3 normal = tangentZ.cross(tangentX);
                    normals[idx] = (normal.length() > 1e-6f) ? normal.normalized() : nuage::Vec3(0, 1, 0);
                }
            }

            auto appendVertex = [&](int idx) {
                const auto& pos = positions[idx];
                const auto& normal = normals[idx];
                float t = (pos.y - cfg.heightMin) / heightRange;
                nuage::Vec3 color = heightColor(t);
                verts.insert(verts.end(), {
                    pos.x, pos.y, pos.z,
                    normal.x, normal.y, normal.z,
                    color.x, color.y, color.z
                });
            };

            for (int z = 0; z < resZ - 1; ++z) {
                for (int x = 0; x < resX - 1; ++x) {
                    int i00 = z * resX + x;
                    int i10 = i00 + 1;
                    int i01 = i00 + resX;
                    int i11 = i01 + 1;

                    appendVertex(i00);
                    appendVertex(i10);
                    appendVertex(i11);

                    appendVertex(i00);
                    appendVertex(i11);
                    appendVertex(i01);
                }
            }

            std::filesystem::path meshPath = tilesDir / ("tile_" + std::to_string(tx) + "_" + std::to_string(ty) + ".mesh");
            if (!writeMesh(meshPath, verts)) {
                std::cerr << "Failed to write mesh: " << meshPath << "\n";
                return 1;
            }

            std::filesystem::path metaPath = tilesDir / ("tile_" + std::to_string(tx) + "_" + std::to_string(ty) + ".meta.json");
            writeTileMeta(metaPath, tx, ty, localMinH, localMaxH, cfg.gridResolution);

            if (cfg.maskResolution > 0 && !allPolys.empty()) {
                std::vector<std::uint8_t> mask(static_cast<size_t>(cfg.maskResolution * cfg.maskResolution), 0);
                auto bucketIt = polyBuckets.find(tileKey(tx, ty));
                if (bucketIt != polyBuckets.end()) {
                    rasterizePolygonsToMaskList(mask, cfg.maskResolution, tileMinX, tileMinZ,
                                                cfg.tileSize, allPolys, bucketIt->second);
                }
                if (cfg.maskSmooth > 0) {
                    smoothMask(mask, cfg.maskResolution, cfg.maskSmooth);
                }

                std::filesystem::path maskPath = tilesDir / ("tile_" + std::to_string(tx) + "_" + std::to_string(ty) + ".mask");
                if (!writeMask(maskPath, mask)) {
                    std::cerr << "Failed to write mask: " << maskPath << "\n";
                    return 1;
                }
            }

            tileIndex.emplace_back(tx, ty);
        }
    }

    std::filesystem::path manifestPath = outDir / "manifest.json";
    std::ofstream manifest(manifestPath);
    constexpr double kDegToRad = 3.141592653589793 / 180.0;
    double metersPerLat = 111320.0;
    double metersPerLon = 111320.0 * std::cos(cfg.originLat * kDegToRad);
    manifest << "{\n";
    manifest << "  \"version\": \"1.0\",\n";
    manifest << "  \"originLLA\": [" << cfg.originLat << ", " << cfg.originLon << ", " << cfg.originAlt << "],\n";
    manifest << "  \"projection\": {\n";
    manifest << "    \"type\": \"equirectangular\",\n";
    manifest << "    \"lat0\": " << cfg.originLat << ",\n";
    manifest << "    \"lon0\": " << cfg.originLon << ",\n";
    manifest << "    \"metersPerLat\": " << metersPerLat << ",\n";
    manifest << "    \"metersPerLon\": " << metersPerLon << "\n";
    manifest << "  },\n";
    manifest << "  \"enuBasis\": [\"east\", \"up\", \"north\"],\n";
    manifest << "  \"tileSizeMeters\": " << cfg.tileSize << ",\n";
    manifest << "  \"gridResolution\": " << cfg.gridResolution << ",\n";
    manifest << "  \"heightScaleMeters\": 1.0,\n";
    manifest << "  \"boundsENU\": [" << minX << ", " << minZ << ", " << maxX << ", " << maxZ << "],\n";
    if (cfg.maskResolution > 0 && !cfg.osmPath.empty()) {
        manifest << "  \"maskResolution\": " << cfg.maskResolution << ",\n";
        manifest << "  \"availableLayers\": [\"height\", \"mask\"],\n";
    } else {
        manifest << "  \"availableLayers\": [\"height\"],\n";
    }
    manifest << "  \"tileCount\": " << tileIndex.size() << ",\n";
    manifest << "  \"tileIndex\": [\n";
    for (size_t i = 0; i < tileIndex.size(); ++i) {
        manifest << "    [" << tileIndex[i].first << ", " << tileIndex[i].second << "]";
        manifest << (i + 1 < tileIndex.size() ? ",\n" : "\n");
    }
    manifest << "  ],\n";
    manifest << "  \"compilerInfo\": {\"name\": \"terrainc\"}\n";
    manifest << "}\n";

    std::cout << "Wrote " << tileIndex.size() << " tiles to " << outDir << "\n";
    return 0;
}
