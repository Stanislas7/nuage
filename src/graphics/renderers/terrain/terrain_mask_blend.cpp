#include "graphics/renderers/terrain/terrain_mask_blend.hpp"
#include <algorithm>

namespace nuage {

void apply_mask_to_verts(std::vector<float>& verts, const std::vector<std::uint8_t>& mask,
                         int maskRes, float tileSize, float tileMinX, float tileMinZ,
                         const std::array<std::uint8_t, 256>* classFlags) {
    if (maskRes <= 0 || mask.empty()) {
        return;
    }
    int stride = 9;
    size_t vertexCount = verts.size() / stride;
    for (size_t i = 0; i < vertexCount; ++i) {
        float px = verts[i * stride + 0];
        float pz = verts[i * stride + 2];
        float fx = (px - tileMinX) / tileSize;
        float fz = (pz - tileMinZ) / tileSize;
        float mx = fx * (maskRes - 1);
        float mz = fz * (maskRes - 1);
        int x0 = std::clamp(static_cast<int>(std::floor(mx)), 0, maskRes - 1);
        int z0 = std::clamp(static_cast<int>(std::floor(mz)), 0, maskRes - 1);
        int x1 = std::min(x0 + 1, maskRes - 1);
        int z1 = std::min(z0 + 1, maskRes - 1);
        float tx = mx - static_cast<float>(x0);
        float tz = mz - static_cast<float>(z0);

        auto clsAt = [&](int x, int z) {
            return mask[static_cast<size_t>(z) * maskRes + static_cast<size_t>(x)];
        };

        float w00 = (1.0f - tx) * (1.0f - tz);
        float w10 = tx * (1.0f - tz);
        float w01 = (1.0f - tx) * tz;
        float w11 = tx * tz;

        // Store water/urban/forest weights in vertex color for texture blending.
        float water = 0.0f;
        float urban = 0.0f;
        float forest = 0.0f;

        auto accumulate = [&](std::uint8_t cls, float w) {
            if (classFlags) {
                std::uint8_t flags = (*classFlags)[cls];
                if (flags & 0x1) water += w;
                if (flags & 0x2) urban += w;
                if (flags & 0x4) forest += w;
                return;
            }
            switch (cls) {
                case 1: water += w; break;
                case 2: urban += w; break;
                case 3: forest += w; break;
                default: break;
            }
        };

        accumulate(clsAt(x0, z0), w00);
        accumulate(clsAt(x1, z0), w10);
        accumulate(clsAt(x0, z1), w01);
        accumulate(clsAt(x1, z1), w11);

        verts[i * stride + 6] = water;
        verts[i * stride + 7] = urban;
        verts[i * stride + 8] = forest;
    }
}

} // namespace nuage
