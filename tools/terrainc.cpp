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
    std::string landcoverPath;
    std::string landclassPath;
    std::string landclassMapPath;
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
    float roadWidthBoost = 1.8f;
    int roadSmooth = 0;
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
              << "                --xmax <lon> --ymax <lat> --mask-smooth <passes>\n"
              << "                --road-width-boost <scale> --road-smooth <passes>]\n"
              << "               [--landcover <path>] [--landclass <path>] [--landclass-map <path>]\n";
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
        } else if (arg == "--landcover") {
            if (!next(cfg.landcoverPath)) return false;
        } else if (arg == "--landclass") {
            if (!next(cfg.landclassPath)) return false;
        } else if (arg == "--landclass-map") {
            if (!next(cfg.landclassMapPath)) return false;
        } else if (arg == "--mask-res") {
            std::string v;
            if (!next(v)) return false;
            cfg.maskResolution = std::stoi(v);
        } else if (arg == "--mask-smooth") {
            std::string v;
            if (!next(v)) return false;
            cfg.maskSmooth = std::stoi(v);
        } else if (arg == "--road-width-boost") {
            std::string v;
            if (!next(v)) return false;
            cfg.roadWidthBoost = std::stof(v);
        } else if (arg == "--road-smooth") {
            std::string v;
            if (!next(v)) return false;
            cfg.roadSmooth = std::stoi(v);
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

struct Road {
    std::vector<nuage::Vec2> points;
    float minX = 0.0f;
    float minZ = 0.0f;
    float maxX = 0.0f;
    float maxZ = 0.0f;
    float halfWidth = 3.0f;
    bool valid = false;
};

struct GeoTransform {
    double originX = 0.0;
    double pixelW = 0.0;
    double rotX = 0.0;
    double originY = 0.0;
    double rotY = 0.0;
    double pixelH = 0.0;
};

struct LandcoverRaster {
    int width = 0;
    int height = 0;
    std::vector<std::uint16_t> data;
    GeoTransform geo;
    bool valid = false;
};

struct LandclassMap {
    std::unordered_map<int, int> values;
    int defaultValue = 0;
    bool enabled = false;
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

bool parseGeoTransform(const std::string& xml, GeoTransform& out) {
    const std::string startTag = "<GeoTransform>";
    const std::string endTag = "</GeoTransform>";
    auto start = xml.find(startTag);
    auto end = xml.find(endTag);
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return false;
    }
    start += startTag.size();
    std::string content = xml.substr(start, end - start);
    std::vector<double> values;
    std::stringstream ss(content);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::stringstream ts(token);
        double v = 0.0;
        if (!(ts >> v)) {
            return false;
        }
        values.push_back(v);
    }
    if (values.size() != 6) {
        return false;
    }
    out.originX = values[0];
    out.pixelW = values[1];
    out.rotX = values[2];
    out.originY = values[3];
    out.rotY = values[4];
    out.pixelH = values[5];
    return true;
}

bool readGeoTransformFromAux(const std::string& rasterPath, GeoTransform& out) {
    std::filesystem::path auxPath = rasterPath + ".aux.xml";
    std::ifstream in(auxPath);
    if (!in.is_open()) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return parseGeoTransform(buffer.str(), out);
}

LandcoverRaster loadLandcoverRaster(const std::string& path) {
    LandcoverRaster lc;
    if (path.empty()) {
        return lc;
    }
    GeoTransform gt;
    if (!readGeoTransformFromAux(path, gt)) {
        std::cerr << "Landcover requires a GeoTransform in " << path << ".aux.xml\n";
        return lc;
    }
    if (std::abs(gt.rotX) > 1e-6 || std::abs(gt.rotY) > 1e-6) {
        std::cerr << "Landcover rotation not supported; please use north-up rasters.\n";
        return lc;
    }
    int w = 0;
    int h = 0;
    int ch = 0;
    if (stbi_is_16_bit(path.c_str())) {
        std::uint16_t* data = stbi_load_16(path.c_str(), &w, &h, &ch, 1);
        if (!data) {
            std::cerr << "Failed to load landcover: " << path << "\n";
            return lc;
        }
        lc.data.assign(data, data + static_cast<size_t>(w * h));
        stbi_image_free(data);
    } else {
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 1);
        if (!data) {
            std::cerr << "Failed to load landcover: " << path << "\n";
            return lc;
        }
        lc.data.resize(static_cast<size_t>(w * h));
        for (int i = 0; i < w * h; ++i) {
            lc.data[static_cast<size_t>(i)] = data[i];
        }
        stbi_image_free(data);
    }
    lc.width = w;
    lc.height = h;
    lc.geo = gt;
    lc.valid = !lc.data.empty();
    return lc;
}

LandclassMap loadLandclassMap(const std::string& path) {
    LandclassMap mapping;
    if (path.empty()) {
        return mapping;
    }
    auto jsonOpt = nuage::loadJsonConfig(path);
    if (!jsonOpt) {
        std::cerr << "Failed to load landclass map: " << path << "\n";
        return mapping;
    }
    const auto& j = *jsonOpt;
    if (j.contains("default") && j["default"].is_number_integer()) {
        mapping.defaultValue = j["default"].get<int>();
    }
    const nlohmann::json* mapNode = nullptr;
    if (j.contains("map") && j["map"].is_object()) {
        mapNode = &j["map"];
    } else if (j.is_object()) {
        mapNode = &j;
    }
    if (mapNode) {
        for (auto it = mapNode->begin(); it != mapNode->end(); ++it) {
            if (!it.value().is_number_integer()) {
                continue;
            }
            int key = 0;
            try {
                key = std::stoi(it.key());
            } catch (...) {
                continue;
            }
            mapping.values[key] = it.value().get<int>();
        }
    }
    mapping.enabled = !mapping.values.empty();
    return mapping;
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
            return 5;
        }
        if (v == "cemetery" || v == "allotments" || v == "recreation_ground") {
            return 4;
        }
        return 5;
    }
    std::string natural = getTag("natural");
    if (!natural.empty()) {
        std::string v = natural;
        if (v == "wood" || v == "forest") {
            return 3;
        }
        if (v == "grassland" || v == "scrub" || v == "heath") {
            return 5;
        }
        if (v == "bare_rock" || v == "scree" || v == "shingle" || v == "sand" || v == "beach") {
            return 6;
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

void unprojectToLonLat(const Projection& proj, float worldX, float worldZ, double& lon, double& lat) {
    lon = static_cast<double>(worldX) / proj.metersPerLon + proj.lon0;
    lat = static_cast<double>(worldZ) / proj.metersPerLat + proj.lat0;
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

int classPriority(int classId) {
    switch (classId) {
        case 7: return 6; // roads
        case 1: return 6; // water
        case 2: return 5; // urban
        case 6: return 4; // rock/bare
        case 3: return 3; // forest
        case 5: return 2; // farmland/scrub
        case 4: return 1; // grass
        default: return 0;
    }
}

int landcoverClassFromValue(std::uint16_t v) {
    if (v == 0) return 0;
    if (v <= 6) return static_cast<int>(v);
    switch (v) {
        // ESA WorldCover 10m classes.
        case 10: return 3; // tree cover
        case 20: return 5; // shrubland
        case 30: return 4; // grassland
        case 40: return 5; // cropland
        case 50: return 2; // built-up
        case 60: return 6; // bare/sparse
        case 70: return 6; // snow/ice
        case 80: return 1; // permanent water
        case 90: return 1; // wetlands
        case 95: return 1; // wetlands/mangroves
        case 100: return 6; // moss/lichen
        // NLCD (US) common classes.
        case 11: return 1; // open water
        case 21: case 22: case 23: case 24: return 2; // developed
        case 31: case 32: return 6; // barren
        case 41: case 42: case 43: return 3; // forest
        case 52: return 5; // shrub
        case 71: case 72: case 73: case 74: return 4; // grass/herbaceous
        case 81: case 82: return 5; // pasture/crops
        default: return 0;
    }
}

int sampleLandcoverClass(const LandcoverRaster& lc, double lon, double lat) {
    if (!lc.valid || lc.width <= 0 || lc.height <= 0) {
        return 0;
    }
    double px = (lon - lc.geo.originX) / lc.geo.pixelW;
    double py = (lat - lc.geo.originY) / lc.geo.pixelH;
    int ix = static_cast<int>(std::floor(px + 0.5));
    int iy = static_cast<int>(std::floor(py + 0.5));
    if (ix < 0 || iy < 0 || ix >= lc.width || iy >= lc.height) {
        return 0;
    }
    std::uint16_t v = lc.data[static_cast<size_t>(iy) * lc.width + static_cast<size_t>(ix)];
    return landcoverClassFromValue(v);
}

int sampleLandclassValue(const LandcoverRaster& lc, double lon, double lat) {
    if (!lc.valid || lc.width <= 0 || lc.height <= 0) {
        return 0;
    }
    double px = (lon - lc.geo.originX) / lc.geo.pixelW;
    double py = (lat - lc.geo.originY) / lc.geo.pixelH;
    int ix = static_cast<int>(std::floor(px + 0.5));
    int iy = static_cast<int>(std::floor(py + 0.5));
    if (ix < 0 || iy < 0 || ix >= lc.width || iy >= lc.height) {
        return 0;
    }
    std::uint16_t v = lc.data[static_cast<size_t>(iy) * lc.width + static_cast<size_t>(ix)];
    return static_cast<int>(std::min<std::uint16_t>(v, 255));
}

int mapLandclassValue(int value, const LandclassMap* map) {
    if (!map || !map->enabled) {
        return value;
    }
    auto it = map->values.find(value);
    if (it != map->values.end()) {
        return it->second;
    }
    return map->defaultValue;
}

void fillMaskFromLandcover(std::vector<std::uint8_t>& mask, int maskRes,
                           float tileMinX, float tileMinZ, float tileSize,
                           const Projection& proj, const LandcoverRaster& lc) {
    if (!lc.valid || maskRes <= 0) {
        return;
    }
    for (int z = 0; z < maskRes; ++z) {
        for (int x = 0; x < maskRes; ++x) {
            float fx = (static_cast<float>(x) + 0.5f) / maskRes;
            float fz = (static_cast<float>(z) + 0.5f) / maskRes;
            float worldX = tileMinX + fx * tileSize;
            float worldZ = tileMinZ + fz * tileSize;
            double lon = 0.0;
            double lat = 0.0;
            unprojectToLonLat(proj, worldX, worldZ, lon, lat);
            int cls = sampleLandcoverClass(lc, lon, lat);
            mask[z * maskRes + x] = static_cast<std::uint8_t>(cls);
        }
    }
}

void fillMaskFromLandclass(std::vector<std::uint8_t>& mask, int maskRes,
                           float tileMinX, float tileMinZ, float tileSize,
                           const Projection& proj, const LandcoverRaster& lc,
                           const LandclassMap* map) {
    if (!lc.valid || maskRes <= 0) {
        return;
    }
    for (int z = 0; z < maskRes; ++z) {
        for (int x = 0; x < maskRes; ++x) {
            float fx = (static_cast<float>(x) + 0.5f) / maskRes;
            float fz = (static_cast<float>(z) + 0.5f) / maskRes;
            float worldX = tileMinX + fx * tileSize;
            float worldZ = tileMinZ + fz * tileSize;
            double lon = 0.0;
            double lat = 0.0;
            unprojectToLonLat(proj, worldX, worldZ, lon, lat);
            int cls = sampleLandclassValue(lc, lon, lat);
            cls = mapLandclassValue(cls, map);
            cls = std::clamp(cls, 0, 255);
            mask[z * maskRes + x] = static_cast<std::uint8_t>(cls);
        }
    }
}

float roadHalfWidthFromTags(const nlohmann::json& tags) {
    auto getTag = [&](const char* key) -> std::string {
        if (!tags.contains(key)) return {};
        const auto& val = tags[key];
        if (!val.is_string()) return {};
        return val.get<std::string>();
    };
    std::string highway = getTag("highway");
    if (highway.empty()) {
        return 0.0f;
    }
    if (highway == "footway" || highway == "path" || highway == "cycleway" || highway == "bridleway") {
        return 0.0f;
    }
    if (highway == "motorway" || highway == "trunk") return 6.0f;
    if (highway == "primary") return 5.0f;
    if (highway == "secondary") return 4.0f;
    if (highway == "tertiary") return 3.5f;
    if (highway == "residential" || highway == "unclassified") return 3.0f;
    if (highway == "service" || highway == "track") return 2.5f;
    return 3.0f;
}

std::vector<Road> loadRoadsFromGeoJson(const std::string& path, const Projection& proj) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Failed to open roads GeoJSON: " << path << "\n";
        return {};
    }
    nlohmann::json doc;
    in >> doc;
    if (!doc.contains("features") || !doc["features"].is_array()) {
        return {};
    }

    std::vector<Road> roads;
    for (const auto& feature : doc["features"]) {
        if (!feature.contains("geometry") || !feature.contains("properties")) {
            continue;
        }
        const auto& geom = feature["geometry"];
        if (!geom.contains("type") || !geom.contains("coordinates")) {
            continue;
        }
        const auto& props = feature["properties"];
        const auto* tags = &props;
        if (props.contains("tags") && props["tags"].is_object()) {
            tags = &props["tags"];
        }
        float halfWidth = roadHalfWidthFromTags(*tags);
        if (halfWidth <= 0.0f) {
            continue;
        }

        std::string type = geom["type"].get<std::string>();
        const auto& coords = geom["coordinates"];
        auto addLine = [&](const nlohmann::json& line) {
            if (!line.is_array() || line.size() < 2) return;
            Road road;
            road.halfWidth = halfWidth;
            road.minX = road.minZ = std::numeric_limits<float>::max();
            road.maxX = road.maxZ = std::numeric_limits<float>::lowest();
            for (const auto& pt : line) {
                if (!pt.is_array() || pt.size() < 2) continue;
                double lon = pt[0].get<double>();
                double lat = pt[1].get<double>();
                nuage::Vec2 p = projectLonLat(proj, lon, lat);
                road.minX = std::min(road.minX, p.x);
                road.maxX = std::max(road.maxX, p.x);
                road.minZ = std::min(road.minZ, p.y);
                road.maxZ = std::max(road.maxZ, p.y);
                road.points.push_back(p);
            }
            if (road.points.size() >= 2) {
                road.valid = true;
                roads.push_back(std::move(road));
            }
        };

        if (type == "LineString") {
            addLine(coords);
        } else if (type == "MultiLineString") {
            if (coords.is_array()) {
                for (const auto& line : coords) {
                    addLine(line);
                }
            }
        }
    }
    return roads;
}

float distancePointToSegment(const nuage::Vec2& p, const nuage::Vec2& a, const nuage::Vec2& b) {
    nuage::Vec2 ab = b - a;
    float denom = ab.x * ab.x + ab.y * ab.y;
    if (denom <= 1e-6f) {
        return (p - a).length();
    }
    float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / denom;
    t = std::clamp(t, 0.0f, 1.0f);
    nuage::Vec2 proj = a + ab * t;
    return (p - proj).length();
}

void rasterizeRoadsToMaskList(std::vector<std::uint8_t>& mask, int maskRes,
                              float tileMinX, float tileMinZ, float tileSize,
                              const std::vector<Road>& roads,
                              const std::vector<int>& indices,
                              float widthBoost) {
    if (maskRes <= 0 || indices.empty()) return;
    for (int idx : indices) {
        const auto& road = roads[static_cast<size_t>(idx)];
        if (!road.valid) continue;
        if (road.maxX <= tileMinX || road.minX >= tileMinX + tileSize ||
            road.maxZ <= tileMinZ || road.minZ >= tileMinZ + tileSize) {
            continue;
        }
        float halfWidth = road.halfWidth * widthBoost;
        float expand = halfWidth;
        int x0 = static_cast<int>(std::floor((road.minX - expand - tileMinX) / tileSize * maskRes));
        int x1 = static_cast<int>(std::ceil((road.maxX + expand - tileMinX) / tileSize * maskRes));
        int z0 = static_cast<int>(std::floor((road.minZ - expand - tileMinZ) / tileSize * maskRes));
        int z1 = static_cast<int>(std::ceil((road.maxZ + expand - tileMinZ) / tileSize * maskRes));
        x0 = std::clamp(x0, 0, maskRes - 1);
        x1 = std::clamp(x1, 0, maskRes - 1);
        z0 = std::clamp(z0, 0, maskRes - 1);
        z1 = std::clamp(z1, 0, maskRes - 1);

        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                float fx = (static_cast<float>(x) + 0.5f) / maskRes;
                float fz = (static_cast<float>(z) + 0.5f) / maskRes;
                nuage::Vec2 p(tileMinX + fx * tileSize, tileMinZ + fz * tileSize);
                bool hit = false;
                for (size_t i = 0; i + 1 < road.points.size(); ++i) {
                    float d = distancePointToSegment(p, road.points[i], road.points[i + 1]);
                    if (d <= halfWidth) {
                        hit = true;
                        break;
                    }
                }
                if (hit) {
                    std::uint8_t& cell = mask[z * maskRes + x];
                    if (cell == 1) continue;
                    if (classPriority(7) >= classPriority(cell)) {
                        cell = 7;
                    }
                }
            }
        }
    }
}

void rasterizePolygonsToMask(std::vector<std::uint8_t>& mask, int maskRes,
                             float tileMinX, float tileMinZ, float tileSize,
                             const std::vector<Polygon>& polys, int targetClass) {
    if (maskRes <= 0) return;
    for (const auto& poly : polys) {
        if (poly.classId != targetClass) continue;
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
                    if (targetClass == 1 || classPriority(targetClass) >= classPriority(cell)) {
                        cell = static_cast<std::uint8_t>(targetClass);
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
                    if (poly.classId == 1 || classPriority(poly.classId) >= classPriority(cell)) {
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

    if (cfg.maskResolution > 0 && cfg.osmPath.empty()
        && cfg.landcoverPath.empty() && cfg.landclassPath.empty()) {
        std::cerr << "Mask resolution set but no OSM, landcover, or landclass file provided.\n";
        return 1;
    }
    if (!cfg.landcoverPath.empty() && cfg.maskResolution <= 0) {
        std::cerr << "Landcover provided but mask resolution not set.\n";
        return 1;
    }
    if (!cfg.landclassPath.empty() && cfg.maskResolution <= 0) {
        std::cerr << "Landclass provided but mask resolution not set.\n";
        return 1;
    }
    if (!cfg.osmPath.empty() && !cfg.hasBbox) {
        std::cerr << "OSM provided but bbox missing; use --xmin/--ymin/--xmax/--ymax.\n";
        return 1;
    }
    if (!cfg.landcoverPath.empty() && !cfg.hasBbox) {
        std::cerr << "Landcover provided but bbox missing; use --xmin/--ymin/--xmax/--ymax.\n";
        return 1;
    }
    if (!cfg.landclassPath.empty() && !cfg.hasBbox) {
        std::cerr << "Landclass provided but bbox missing; use --xmin/--ymin/--xmax/--ymax.\n";
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

    LandcoverRaster landcover = loadLandcoverRaster(cfg.landcoverPath);
    if (!cfg.landcoverPath.empty() && !landcover.valid) {
        return 1;
    }
    LandcoverRaster landclass = loadLandcoverRaster(cfg.landclassPath);
    if (!cfg.landclassPath.empty() && !landclass.valid) {
        return 1;
    }
    bool useLandclass = landclass.valid;
    LandclassMap landclassMap = loadLandclassMap(cfg.landclassMapPath);
    if (!cfg.landclassMapPath.empty() && !landclassMap.enabled) {
        std::cerr << "Landclass map provided but no valid mappings found.\n";
    }
    if (useLandclass && (landcover.valid || !cfg.osmPath.empty())) {
        std::cerr << "Landclass provided; ignoring landcover and OSM masks.\n";
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
    std::vector<Road> roadLines;
    std::unordered_map<std::int64_t, std::vector<int>> polyBuckets;
    std::unordered_map<std::int64_t, std::vector<int>> roadBuckets;
    if (!cfg.osmPath.empty()) {
        std::filesystem::path outDir(cfg.outDir);
        std::filesystem::path tmpDir = outDir / "osm_tmp";
        std::filesystem::create_directories(tmpDir);

        std::filesystem::path areaPbf = tmpDir / "area.pbf";
        std::filesystem::path waterPbf = tmpDir / "water.pbf";
        std::filesystem::path landPbf = tmpDir / "landuse.pbf";
        std::filesystem::path waterGeo = tmpDir / "water.geojson";
        std::filesystem::path landGeo = tmpDir / "landuse.geojson";
        std::filesystem::path roadPbf = tmpDir / "roads.pbf";
        std::filesystem::path roadGeo = tmpDir / "roads.geojson";

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
                 << " w/waterway=river w/waterway=stream w/waterway=canal"
                 << " r/natural=water r/waterway=riverbank r/water=* r/natural=wetland"
                 << " r/waterway=river r/waterway=stream r/waterway=canal";
        if (!runCommand(waterCmd.str())) {
            std::cerr << "Failed to filter water from OSM.\n";
            return 1;
        }

        std::ostringstream landCmd;
        landCmd << "osmium tags-filter -o \"" << landPbf.string() << "\" \"" << areaPbf.string()
                << "\" w/landuse=* w/natural=wood w/natural=grassland w/natural=heath w/natural=scrub"
                << " w/natural=beach w/natural=bare_rock w/natural=scree w/natural=shingle"
                << " r/landuse=* r/natural=wood r/natural=grassland r/natural=heath r/natural=scrub"
                << " r/natural=beach r/natural=bare_rock r/natural=scree r/natural=shingle";
        if (!runCommand(landCmd.str())) {
            std::cerr << "Failed to filter landuse from OSM.\n";
            return 1;
        }
        std::ostringstream roadCmd;
        roadCmd << "osmium tags-filter -o \"" << roadPbf.string() << "\" \"" << areaPbf.string()
                << "\" w/highway=* r/highway=*";
        if (!runCommand(roadCmd.str())) {
            std::cerr << "Failed to filter roads from OSM.\n";
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
        std::ostringstream roadExport;
        roadExport << "osmium export -f geojson -o \"" << roadGeo.string() << "\" \"" << roadPbf.string() << "\"";
        if (!runCommand(roadExport.str())) {
            std::cerr << "Failed to export roads GeoJSON.\n";
            return 1;
        }

        landPolys = loadPolygonsFromGeoJson(landGeo.string(), 0, proj, true);
        waterPolys = loadPolygonsFromGeoJson(waterGeo.string(), 1, proj, false);
        roadLines = loadRoadsFromGeoJson(roadGeo.string(), proj);
        std::cout << "Loaded landuse polygons: " << landPolys.size() << "\n";
        std::cout << "Loaded water polygons: " << waterPolys.size() << "\n";
        std::cout << "Loaded road lines: " << roadLines.size() << "\n";
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
    if (!roadLines.empty()) {
        for (size_t i = 0; i < roadLines.size(); ++i) {
            const auto& road = roadLines[i];
            if (!road.valid) continue;
            int minTx = static_cast<int>(std::floor(road.minX / cfg.tileSize));
            int maxTx = static_cast<int>(std::floor(road.maxX / cfg.tileSize));
            int minTz = static_cast<int>(std::floor(road.minZ / cfg.tileSize));
            int maxTz = static_cast<int>(std::floor(road.maxZ / cfg.tileSize));

            minTx = std::max(minTx, minTileX);
            maxTx = std::min(maxTx, maxTileX);
            minTz = std::max(minTz, minTileZ);
            maxTz = std::min(maxTz, maxTileZ);

            for (int ty = minTz; ty <= maxTz; ++ty) {
                for (int tx = minTx; tx <= maxTx; ++tx) {
                    roadBuckets[tileKey(tx, ty)].push_back(static_cast<int>(i));
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

            if (cfg.maskResolution > 0 && (useLandclass || landcover.valid || !allPolys.empty())) {
                std::vector<std::uint8_t> mask(static_cast<size_t>(cfg.maskResolution * cfg.maskResolution), 0);
                if (useLandclass) {
                    fillMaskFromLandclass(mask, cfg.maskResolution, tileMinX, tileMinZ,
                                          cfg.tileSize, proj, landclass, landclassMap.enabled ? &landclassMap : nullptr);
                } else {
                    if (landcover.valid) {
                        fillMaskFromLandcover(mask, cfg.maskResolution, tileMinX, tileMinZ,
                                              cfg.tileSize, proj, landcover);
                    }
                    auto bucketIt = polyBuckets.find(tileKey(tx, ty));
                    if (bucketIt != polyBuckets.end()) {
                        rasterizePolygonsToMaskList(mask, cfg.maskResolution, tileMinX, tileMinZ,
                                                    cfg.tileSize, allPolys, bucketIt->second);
                    }
                    if (cfg.maskSmooth > 0) {
                        smoothMask(mask, cfg.maskResolution, cfg.maskSmooth);
                    }
                    auto roadIt = roadBuckets.find(tileKey(tx, ty));
                    if (roadIt != roadBuckets.end()) {
                        rasterizeRoadsToMaskList(mask, cfg.maskResolution, tileMinX, tileMinZ,
                                                 cfg.tileSize, roadLines, roadIt->second, cfg.roadWidthBoost);
                    }
                    if (cfg.roadSmooth > 0) {
                        smoothMask(mask, cfg.maskResolution, cfg.roadSmooth);
                    }
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
    if (cfg.maskResolution > 0 && (useLandclass || !cfg.osmPath.empty() || landcover.valid)) {
        manifest << "  \"maskResolution\": " << cfg.maskResolution << ",\n";
        manifest << "  \"maskType\": \"" << (useLandclass ? "landclass" : "landuse") << "\",\n";
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
