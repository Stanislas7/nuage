#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace nuage {

void apply_mask_to_verts(std::vector<float>& verts, const std::vector<std::uint8_t>& mask,
                         int maskRes, float tileSize, float tileMinX, float tileMinZ,
                         const std::array<std::uint8_t, 256>* classFlags = nullptr);

} // namespace nuage
