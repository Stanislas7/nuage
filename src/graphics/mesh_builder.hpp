#pragma once

#include "mesh.hpp"
#include "math/vec3.hpp"
#include <vector>
#include <string>

namespace nuage {

struct AircraftMeshSpecs {
    float fuselageLength = 4.0f;
    float wingspan = 6.0f;
    Vec3 bodyColor = {0.8f, 0.2f, 0.2f};
    Vec3 wingColor = {0.3f, 0.3f, 0.4f};
};

class MeshBuilder {
public:
    static std::vector<float> aircraft(const AircraftMeshSpecs& specs);
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
