#include "graphics/renderers/terrain/terrain_mask_blend.hpp"
#include <algorithm>

namespace nuage {

Vec3 terrain_mask_class_color(std::uint8_t cls) {
    switch (cls) {
        case 1: return Vec3(0.14f, 0.32f, 0.55f);
        case 2: return Vec3(0.56f, 0.54f, 0.5f);
        case 3: return Vec3(0.2f, 0.42f, 0.22f);
        case 4: return Vec3(0.46f, 0.55f, 0.32f);
        default: return Vec3(1.0f, 1.0f, 1.0f);
    }
}

void apply_mask_to_verts(std::vector<float>& verts, const std::vector<std::uint8_t>& mask,
                         int maskRes, float tileSize, float tileMinX, float tileMinZ) {
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

        float total = 0.0f;
        Vec3 blended(0.0f, 0.0f, 0.0f);

        std::uint8_t c00 = clsAt(x0, z0);
        if (c00 != 0) { blended = blended + terrain_mask_class_color(c00) * w00; total += w00; }
        std::uint8_t c10 = clsAt(x1, z0);
        if (c10 != 0) { blended = blended + terrain_mask_class_color(c10) * w10; total += w10; }
        std::uint8_t c01 = clsAt(x0, z1);
        if (c01 != 0) { blended = blended + terrain_mask_class_color(c01) * w01; total += w01; }
        std::uint8_t c11 = clsAt(x1, z1);
        if (c11 != 0) { blended = blended + terrain_mask_class_color(c11) * w11; total += w11; }

        if (total <= 0.0f) {
            continue;
        }

        Vec3 base(verts[i * stride + 6], verts[i * stride + 7], verts[i * stride + 8]);
        Vec3 maskColor = blended * (1.0f / total);
        Vec3 mixed = base * (1.0f - total) + maskColor * total;
        verts[i * stride + 6] = mixed.x;
        verts[i * stride + 7] = mixed.y;
        verts[i * stride + 8] = mixed.z;
    }
}

} // namespace nuage
