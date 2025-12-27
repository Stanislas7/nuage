#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "utils/stb_image.h"
#include "math/vec2.hpp"
#include "math/vec3.hpp"

namespace {
struct Heightmap {
    int width = 0;
    int height = 0;
    std::vector<std::uint16_t> pixels;
};

bool loadHeightmap(const std::string& path, Heightmap& out) {
    stbi_set_flip_vertically_on_load(0);
    int width = 0;
    int height = 0;
    int channels = 0;

    std::uint16_t* data16 = stbi_load_16(path.c_str(), &width, &height, &channels, 1);
    if (data16) {
        out.width = width;
        out.height = height;
        out.pixels.assign(data16, data16 + (width * height));
        stbi_image_free(data16);
        return true;
    }

    unsigned char* data8 = stbi_load(path.c_str(), &width, &height, &channels, 1);
    if (!data8) {
        std::cerr << "Failed to load heightmap: " << path << "\n";
        return false;
    }
    out.width = width;
    out.height = height;
    out.pixels.resize(width * height);
    for (int i = 0; i < width * height; ++i) {
        out.pixels[i] = static_cast<std::uint16_t>(data8[i]) * 257;
    }
    stbi_image_free(data8);
    return true;
}

float clamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

float bilinearSample(const Heightmap& hm, float x, float y) {
    float fx = std::clamp(x, 0.0f, static_cast<float>(hm.width - 1));
    float fy = std::clamp(y, 0.0f, static_cast<float>(hm.height - 1));

    int x0 = static_cast<int>(std::floor(fx));
    int y0 = static_cast<int>(std::floor(fy));
    int x1 = std::min(x0 + 1, hm.width - 1);
    int y1 = std::min(y0 + 1, hm.height - 1);

    float tx = fx - static_cast<float>(x0);
    float ty = fy - static_cast<float>(y0);

    auto at = [&](int px, int py) -> float {
        return static_cast<float>(hm.pixels[py * hm.width + px]);
    };

    float v00 = at(x0, y0);
    float v10 = at(x1, y0);
    float v01 = at(x0, y1);
    float v11 = at(x1, y1);

    float v0 = v00 + (v10 - v00) * tx;
    float v1 = v01 + (v11 - v01) * tx;
    return v0 + (v1 - v0) * ty;
}

nuage::Vec3 heightColor(float t) {
    t = clamp01(t);
    if (t < 0.3f) {
        float k = t / 0.3f;
        return nuage::Vec3(0.15f + 0.1f * k, 0.35f + 0.3f * k, 0.15f + 0.1f * k);
    }
    if (t < 0.7f) {
        float k = (t - 0.3f) / 0.4f;
        return nuage::Vec3(0.25f + 0.25f * k, 0.55f - 0.15f * k, 0.2f + 0.1f * k);
    }
    float k = (t - 0.7f) / 0.3f;
    return nuage::Vec3(0.55f + 0.35f * k, 0.5f + 0.35f * k, 0.45f + 0.3f * k);
}

struct Config {
    std::string heightmapPath;
    std::string outDir;
    float sizeX = 0.0f;
    float sizeZ = 0.0f;
    float heightMin = 0.0f;
    float heightMax = 1000.0f;
    float tileSize = 2000.0f;
    int gridResolution = 129;
    double originLat = 0.0;
    double originLon = 0.0;
    double originAlt = 0.0;
};

void printUsage() {
    std::cout << "Usage: terrainc --heightmap <path> --size-x <meters> --size-z <meters>\n"
              << "               --height-min <m> --height-max <m> --tile-size <m>\n"
              << "               --grid <cells> --out <dir>\n"
              << "               [--origin-lat <deg>] [--origin-lon <deg>] [--origin-alt <m>]\n";
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

    if (cfg.heightmapPath.empty() || cfg.outDir.empty() || cfg.sizeX <= 0.0f || cfg.sizeZ <= 0.0f) {
        return false;
    }
    if (cfg.tileSize <= 0.0f) cfg.tileSize = 2000.0f;
    cfg.gridResolution = std::max(2, cfg.gridResolution);
    if (cfg.heightMax <= cfg.heightMin) cfg.heightMax = cfg.heightMin + 1.0f;
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

} // namespace

int main(int argc, char** argv) {
    Config cfg;
    if (!parseArgs(argc, argv, cfg)) {
        printUsage();
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

            tileIndex.emplace_back(tx, ty);
        }
    }

    std::filesystem::path manifestPath = outDir / "manifest.json";
    std::ofstream manifest(manifestPath);
    manifest << "{\n";
    manifest << "  \"version\": \"1.0\",\n";
    manifest << "  \"originLLA\": [" << cfg.originLat << ", " << cfg.originLon << ", " << cfg.originAlt << "],\n";
    manifest << "  \"tileSizeMeters\": " << cfg.tileSize << ",\n";
    manifest << "  \"gridResolution\": " << cfg.gridResolution << ",\n";
    manifest << "  \"heightScaleMeters\": 1.0,\n";
    manifest << "  \"boundsENU\": [" << minX << ", " << minZ << ", " << maxX << ", " << maxZ << "],\n";
    manifest << "  \"availableLayers\": [\"height\"],\n";
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
