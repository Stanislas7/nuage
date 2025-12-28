#include "tools/terrainc/mask_smoothing.hpp"
#include <algorithm>

void smoothMask(std::vector<std::uint8_t>& mask, int res, int passes) {
    if (passes <= 0 || res <= 0 || mask.empty()) {
        return;
    }
    std::vector<std::uint8_t> scratch(mask.size());
    for (int pass = 0; pass < passes; ++pass) {
        for (int z = 0; z < res; ++z) {
            for (int x = 0; x < res; ++x) {
                int counts[5] = {0, 0, 0, 0, 0};
                for (int dz = -1; dz <= 1; ++dz) {
                    int sz = std::clamp(z + dz, 0, res - 1);
                    for (int dx = -1; dx <= 1; ++dx) {
                        int sx = std::clamp(x + dx, 0, res - 1);
                        std::uint8_t cls = mask[static_cast<size_t>(sz) * res + static_cast<size_t>(sx)];
                        if (cls <= 4) {
                            counts[cls] += 1;
                        }
                    }
                }
                int best = 0;
                for (int cls = 1; cls <= 4; ++cls) {
                    if (counts[cls] > counts[best]) {
                        best = cls;
                    }
                }
                std::uint8_t current = mask[static_cast<size_t>(z) * res + static_cast<size_t>(x)];
                if (best != 0 && counts[best] >= 5) {
                    scratch[static_cast<size_t>(z) * res + static_cast<size_t>(x)] = static_cast<std::uint8_t>(best);
                } else {
                    scratch[static_cast<size_t>(z) * res + static_cast<size_t>(x)] = current;
                }
            }
        }
        mask.swap(scratch);
    }
}
