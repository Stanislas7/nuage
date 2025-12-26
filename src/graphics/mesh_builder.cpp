#include "graphics/mesh_builder.hpp"
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

std::vector<float> MeshBuilder::aircraft(const AircraftMeshSpecs& specs) {
    std::vector<float> verts;
    float L = specs.fuselageLength;
    float W = specs.wingspan;
    Vec3 bc = specs.bodyColor;
    Vec3 wc = specs.wingColor;
    Vec3 cockpit = {0.3f, 0.5f, 0.7f};  // Blue cockpit
    Vec3 accent = {0.9f, 0.9f, 0.9f};   // White accents

    auto tri = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 col) {
        verts.insert(verts.end(), {a.x, a.y, a.z, col.x, col.y, col.z});
        verts.insert(verts.end(), {b.x, b.y, b.z, col.x, col.y, col.z});
        verts.insert(verts.end(), {c.x, c.y, c.z, col.x, col.y, col.z});
    };

    // Fuselage - tapered body (top)
    tri({0, 0.15f, L*0.5f}, {-0.3f, 0.3f, -L*0.3f}, {0.3f, 0.3f, -L*0.3f}, bc);
    // Fuselage - bottom
    tri({0, -0.1f, L*0.5f}, {0.3f, -0.15f, -L*0.3f}, {-0.3f, -0.15f, -L*0.3f}, bc);
    // Fuselage - left side
    tri({0, 0.15f, L*0.5f}, {-0.3f, -0.15f, -L*0.3f}, {-0.3f, 0.3f, -L*0.3f}, {bc.x*0.8f, bc.y*0.8f, bc.z*0.8f});
    tri({0, 0.15f, L*0.5f}, {0, -0.1f, L*0.5f}, {-0.3f, -0.15f, -L*0.3f}, {bc.x*0.8f, bc.y*0.8f, bc.z*0.8f});
    // Fuselage - right side
    tri({0, 0.15f, L*0.5f}, {0.3f, 0.3f, -L*0.3f}, {0.3f, -0.15f, -L*0.3f}, {bc.x*0.9f, bc.y*0.9f, bc.z*0.9f});
    tri({0, 0.15f, L*0.5f}, {0.3f, -0.15f, -L*0.3f}, {0, -0.1f, L*0.5f}, {bc.x*0.9f, bc.y*0.9f, bc.z*0.9f});

    // Tail section
    tri({-0.3f, 0.3f, -L*0.3f}, {0, 0.1f, -L*0.5f}, {0.3f, 0.3f, -L*0.3f}, bc);
    tri({-0.3f, -0.15f, -L*0.3f}, {0.3f, -0.15f, -L*0.3f}, {0, -0.05f, -L*0.5f}, bc);

    // Cockpit (canopy)
    tri({0, 0.35f, L*0.2f}, {-0.2f, 0.3f, -0.2f}, {0.2f, 0.3f, -0.2f}, cockpit);

    // Main wings - left
    tri({0, 0.1f, 0.3f}, {-W*0.5f, 0.15f, -0.3f}, {0, 0.1f, -0.8f}, wc);
    tri({0, 0.05f, 0.3f}, {0, 0.05f, -0.8f}, {-W*0.5f, 0.1f, -0.3f}, {wc.x*0.85f, wc.y*0.85f, wc.z*0.85f});
    // Main wings - right
    tri({0, 0.1f, 0.3f}, {0, 0.1f, -0.8f}, {W*0.5f, 0.15f, -0.3f}, wc);
    tri({0, 0.05f, 0.3f}, {W*0.5f, 0.1f, -0.3f}, {0, 0.05f, -0.8f}, {wc.x*0.85f, wc.y*0.85f, wc.z*0.85f});

    // Horizontal stabilizer
    tri({0, 0.1f, -L*0.45f}, {-1.0f, 0.12f, -L*0.55f}, {0, 0.1f, -L*0.55f}, wc);
    tri({0, 0.1f, -L*0.45f}, {0, 0.1f, -L*0.55f}, {1.0f, 0.12f, -L*0.55f}, wc);

    // Vertical stabilizer (tail fin)
    tri({0, 0.1f, -L*0.4f}, {0, 0.7f, -L*0.55f}, {0, 0.1f, -L*0.55f}, {bc.x*0.7f, bc.y*0.7f, bc.z*0.7f});

    // Engine cowling accent (white stripe)
    tri({0, 0.2f, L*0.35f}, {-0.15f, 0.25f, L*0.2f}, {0.15f, 0.25f, L*0.2f}, accent);

    return verts;
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
            
            verts.insert(verts.end(), {x0, 0.0f, z0, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z0, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z1, r, g, b});
            
            verts.insert(verts.end(), {x0, 0.0f, z0, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z1, r, g, b});
            verts.insert(verts.end(), {x0, 0.0f, z1, r, g, b});
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

    float invMax = 1.0f / 65535.0f;
    if (heightMax <= heightMin) {
        heightMax = heightMin + 1.0f;
    }

    auto sampleHeight = [&](int x, int z) {
        std::uint16_t v = heightmap.pixels[z * heightmap.width + x];
        float t = static_cast<float>(v) * invMax;
        return heightMin + t * (heightMax - heightMin);
    };

    std::vector<float> verts;
    verts.reserve((resX - 1) * (resZ - 1) * 6 * (textured ? 5 : 6));

    for (int z = 0; z < resZ - 1; ++z) {
        for (int x = 0; x < resX - 1; ++x) {
            int x0 = x * stepX;
            int z0 = z * stepZ;
            int x1 = std::min(x0 + stepX, heightmap.width - 1);
            int z1 = std::min(z0 + stepZ, heightmap.height - 1);

            float fx0 = (resX > 1) ? (static_cast<float>(x) / (resX - 1)) : 0.0f;
            float fz0 = (resZ > 1) ? (static_cast<float>(z) / (resZ - 1)) : 0.0f;
            float fx1 = (resX > 1) ? (static_cast<float>(x + 1) / (resX - 1)) : 1.0f;
            float fz1 = (resZ > 1) ? (static_cast<float>(z + 1) / (resZ - 1)) : 1.0f;

            float px0 = (fx0 - 0.5f) * sizeX;
            float pz0 = (fz0 - 0.5f) * sizeZ;
            float px1 = (fx1 - 0.5f) * sizeX;
            float pz1 = (fz1 - 0.5f) * sizeZ;

            float h00 = sampleHeight(x0, z0);
            float h10 = sampleHeight(x1, z0);
            float h11 = sampleHeight(x1, z1);
            float h01 = sampleHeight(x0, z1);

            if (textured) {
                verts.insert(verts.end(), {px0, h00, pz0, fx0, fz0});
                verts.insert(verts.end(), {px1, h10, pz0, fx1, fz0});
                verts.insert(verts.end(), {px1, h11, pz1, fx1, fz1});

                verts.insert(verts.end(), {px0, h00, pz0, fx0, fz0});
                verts.insert(verts.end(), {px1, h11, pz1, fx1, fz1});
                verts.insert(verts.end(), {px0, h01, pz1, fx0, fz1});
            } else {
                float t00 = (h00 - heightMin) / (heightMax - heightMin);
                float t10 = (h10 - heightMin) / (heightMax - heightMin);
                float t11 = (h11 - heightMin) / (heightMax - heightMin);
                float t01 = (h01 - heightMin) / (heightMax - heightMin);

                Vec3 c00 = heightColor(t00);
                Vec3 c10 = heightColor(t10);
                Vec3 c11 = heightColor(t11);
                Vec3 c01 = heightColor(t01);

                verts.insert(verts.end(), {px0, h00, pz0, c00.x, c00.y, c00.z});
                verts.insert(verts.end(), {px1, h10, pz0, c10.x, c10.y, c10.z});
                verts.insert(verts.end(), {px1, h11, pz1, c11.x, c11.y, c11.z});

                verts.insert(verts.end(), {px0, h00, pz0, c00.x, c00.y, c00.z});
                verts.insert(verts.end(), {px1, h11, pz1, c11.x, c11.y, c11.z});
                verts.insert(verts.end(), {px0, h01, pz1, c01.x, c01.y, c01.z});
            }
        }
    }

    return verts;
}

}
