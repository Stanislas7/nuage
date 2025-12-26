#pragma once

#include "mesh.hpp"
#include "math/vec3.hpp"
#include <vector>
#include <string>

namespace nuage {

class MeshBuilder {
public:
    static std::vector<float> terrain(float size, int subdivisions);
    static std::vector<float> terrainFromHeightmap(const std::string& path,
                                                   float sizeX,
                                                   float sizeZ,
                                                   float heightMin,
                                                   float heightMax,
                                                   int maxResolution,
                                                   bool textured,
                                                   bool flipY = true);
};

}
