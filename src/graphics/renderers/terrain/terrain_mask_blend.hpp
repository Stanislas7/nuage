#pragma once

#include "math/vec3.hpp"
#include <cstdint>
#include <vector>

namespace nuage {

Vec3 terrain_mask_class_color(std::uint8_t cls);
void apply_mask_to_verts(std::vector<float>& verts, const std::vector<std::uint8_t>& mask,
                         int maskRes, float tileSize, float tileMinX, float tileMinZ);

} // namespace nuage
