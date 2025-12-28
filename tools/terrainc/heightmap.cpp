#include "tools/terrainc/heightmap.hpp"
#include "utils/stb_image.h"
#include <algorithm>
#include <cmath>
#include <iostream>

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
