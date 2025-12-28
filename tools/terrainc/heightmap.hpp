#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Heightmap {
    int width = 0;
    int height = 0;
    std::vector<std::uint16_t> pixels;
};

bool loadHeightmap(const std::string& path, Heightmap& out);
float clamp01(float v);
float bilinearSample(const Heightmap& hm, float x, float y);
