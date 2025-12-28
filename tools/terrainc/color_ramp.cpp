#include "tools/terrainc/color_ramp.hpp"
#include "tools/terrainc/heightmap.hpp"

nuage::Vec3 heightColor(float t) {
    t = clamp01(t);
    if (t < 0.3f) {
        float k = t / 0.3f;
        return nuage::Vec3(0.15f + 0.1f * k, 0.35f + 0.3f * k, 0.15f + 0.1f * k);
    }
    if (t < 0.7f) {
        float k = (t - 0.3f) / 0.4f;
        return nuage::Vec3(0.25f + 0.25f * k, 0.55f - 0.15f * k, 0.2f + 0.1f * k);
    }
    float k = (t - 0.7f) / 0.3f;
    return nuage::Vec3(0.55f + 0.35f * k, 0.5f + 0.35f * k, 0.45f + 0.3f * k);
}
