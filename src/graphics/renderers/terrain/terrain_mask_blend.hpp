#pragma once

#include <cstdint>
#include <vector>

namespace nuage {

void apply_mask_to_verts(std::vector<float>& verts, const std::vector<std::uint8_t>& mask,
                         int maskRes, float tileSize, float tileMinX, float tileMinZ);

} // namespace nuage
