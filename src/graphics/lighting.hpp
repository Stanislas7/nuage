#pragma once

#include "math/vec3.hpp"

namespace nuage {

class Shader;

void applyDirectionalLighting(Shader* shader, const Vec3& sunDir);

} // namespace nuage
