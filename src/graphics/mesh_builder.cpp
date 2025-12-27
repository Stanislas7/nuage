#include "graphics/mesh_builder.hpp"
#include "math/vec2.hpp"
#include "utils/stb_image.h"
#include <iostream>
#include <algorithm>
#include <cstdint>

namespace nuage {

namespace {
struct HeightmapData {
    int width = 0;
    int height = 0;
    std::vector<std::uint16_t> pixels;
};

bool loadHeightmap(const std::string& path, HeightmapData& out, bool flipY) {
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
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

Vec3 heightColor(float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    if (t < 0.3f) {
        float k = t / 0.3f;
        return Vec3(0.15f + 0.1f * k, 0.35f + 0.3f * k, 0.15f + 0.1f * k);
    }
    if (t < 0.7f) {
        float k = (t - 0.3f) / 0.4f;
        return Vec3(0.25f + 0.25f * k, 0.55f - 0.15f * k, 0.2f + 0.1f * k);
    }
    float k = (t - 0.7f) / 0.3f;
    return Vec3(0.55f + 0.35f * k, 0.5f + 0.35f * k, 0.45f + 0.3f * k);
}
}

std::vector<float> MeshBuilder::terrain(float size, int subdivisions) {
    std::vector<float> verts;
    float halfSize = size / 2.0f;
    float step = size / static_cast<float>(subdivisions);
    
    for (int i = 0; i < subdivisions; ++i) {
        for (int j = 0; j < subdivisions; ++j) {
            float x0 = -halfSize + i * step;
            float z0 = -halfSize + j * step;
            float x1 = x0 + step;
            float z1 = z0 + step;
            
            float g = 0.25f + 0.1f * ((i + j) % 2);
            float r = g * 0.5f;
            float b = g * 0.4f;
            
            verts.insert(verts.end(), {x0, 0.0f, z0, 0.0f, 1.0f, 0.0f, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z0, 0.0f, 1.0f, 0.0f, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z1, 0.0f, 1.0f, 0.0f, r, g, b});

            verts.insert(verts.end(), {x0, 0.0f, z0, 0.0f, 1.0f, 0.0f, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z1, 0.0f, 1.0f, 0.0f, r, g, b});
            verts.insert(verts.end(), {x0, 0.0f, z1, 0.0f, 1.0f, 0.0f, r, g, b});
        }
    }
    
    return verts;
}

std::vector<float> MeshBuilder::terrainFromHeightmap(const std::string& path,
                                                     float sizeX,
                                                     float sizeZ,
                                                     float heightMin,
                                                     float heightMax,
                                                     int maxResolution,
                                                     bool textured,
                                                     bool flipY) {
    HeightmapData heightmap;
    if (!loadHeightmap(path, heightmap, flipY)) {
        return {};
    }

    if (heightmap.width < 2 || heightmap.height < 2) {
        std::cerr << "Heightmap too small: " << path << "\n";
        return {};
    }

    if (maxResolution < 2) {
        maxResolution = 2;
    }

    int stepX = std::max(1, heightmap.width / (maxResolution - 1));
    int stepZ = std::max(1, heightmap.height / (maxResolution - 1));

    int resX = (heightmap.width - 1) / stepX + 1;
    int resZ = (heightmap.height - 1) / stepZ + 1;

    if (heightMax <= heightMin) {
        heightMax = heightMin + 1.0f;
    }

    float heightRange = heightMax - heightMin;
    auto sampleHeight = [&](int x, int z) {
        std::uint16_t v = heightmap.pixels[z * heightmap.width + x];
        float t = static_cast<float>(v) / 65535.0f;
        return heightMin + t * heightRange;
    };

    std::vector<float> verts;
    if (resX > 1 && resZ > 1) {
        int stride = textured ? 8 : 9;
        verts.reserve((resX - 1) * (resZ - 1) * 6 * stride);
    }

    std::vector<Vec3> positions(resX * resZ);
    std::vector<Vec3> normals(resX * resZ);
    std::vector<Vec2> uvs;
    if (textured) {
        uvs.resize(resX * resZ);
    }

    for (int z = 0; z < resZ; ++z) {
        for (int x = 0; x < resX; ++x) {
            int sampleX = std::min(x * stepX, heightmap.width - 1);
            int sampleZ = std::min(z * stepZ, heightmap.height - 1);
            int idx = z * resX + x;

            float fx = (resX > 1) ? static_cast<float>(x) / (resX - 1) : 0.0f;
            float fz = (resZ > 1) ? static_cast<float>(z) / (resZ - 1) : 0.0f;

            float px = (fx - 0.5f) * sizeX;
            float pz = (fz - 0.5f) * sizeZ;
            float height = sampleHeight(sampleX, sampleZ);

            positions[idx] = Vec3(px, height, pz);
            if (textured) {
                uvs[idx] = Vec2(fx, fz);
            }
        }
    }

    for (int z = 0; z < resZ; ++z) {
        for (int x = 0; x < resX; ++x) {
            int idx = z * resX + x;
            int left = z * resX + std::max(x - 1, 0);
            int right = z * resX + std::min(x + 1, resX - 1);
            int up = std::max(z - 1, 0) * resX + x;
            int down = std::min(z + 1, resZ - 1) * resX + x;

            Vec3 tangentX = positions[right] - positions[left];
            Vec3 tangentZ = positions[down] - positions[up];
            Vec3 normal = tangentZ.cross(tangentX);
            normals[idx] = (normal.length() > 1e-6f) ? normal.normalized() : Vec3(0, 1, 0);
        }
    }

    auto appendVertex = [&](int vertexIdx) {
        const Vec3& pos = positions[vertexIdx];
        const Vec3& normal = normals[vertexIdx];

        if (textured) {
            const Vec2& uv = uvs[vertexIdx];
            verts.insert(verts.end(), {
                pos.x, pos.y, pos.z,
                normal.x, normal.y, normal.z,
                uv.x, uv.y
            });
        } else {
            float normalizedHeight = (pos.y - heightMin) / heightRange;
            normalizedHeight = std::clamp(normalizedHeight, 0.0f, 1.0f);
            Vec3 color = heightColor(normalizedHeight);

            verts.insert(verts.end(), {
                pos.x, pos.y, pos.z,
                normal.x, normal.y, normal.z,
                color.x, color.y, color.z
            });
        }
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

    return verts;
}

}
