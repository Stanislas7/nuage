#pragma once

#include "ui/anchor.hpp"
#include "math/vec3.hpp"

namespace nuage {

struct UIElement {
    Vec3 position = Vec3(0, 0, 0);
    Vec3 color = Vec3(1, 1, 1);
    Anchor anchor = Anchor::TopLeft;
    float padding = 0.0f;
    float scale = 1.0f;

    UIElement& pos(float x, float y) { position = Vec3(x, y, 0); return *this; }
    UIElement& colorR(float r, float g, float b) { color = Vec3(r, g, b); return *this; }
    UIElement& anchorMode(Anchor a) { anchor = a; return *this; }
    UIElement& paddingValue(float p) { padding = p; return *this; }
    UIElement& scaleVal(float s) { scale = s; return *this; }
};

}
