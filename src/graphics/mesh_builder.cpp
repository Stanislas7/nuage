#include "graphics/mesh_builder.hpp"

namespace nuage {

std::vector<float> MeshBuilder::aircraft(const AircraftMeshSpecs& specs) {
    // Simple aircraft mesh with triangles
    // Format: x, y, z, r, g, b
    std::vector<float> verts;
    
    // Fuselage (Triangle)
    // Nose
    verts.insert(verts.end(), {0.0f, 0.0f, specs.fuselageLength/2.0f, specs.bodyColor.x, specs.bodyColor.y, specs.bodyColor.z});
    // Tail left
    verts.insert(verts.end(), {-0.5f, 0.0f, -specs.fuselageLength/2.0f, specs.bodyColor.x, specs.bodyColor.y, specs.bodyColor.z});
    // Tail right
    verts.insert(verts.end(), {0.5f, 0.0f, -specs.fuselageLength/2.0f, specs.bodyColor.x, specs.bodyColor.y, specs.bodyColor.z});
    
    // Wings (Triangle)
    // Center
    verts.insert(verts.end(), {0.0f, 0.2f, 0.0f, specs.wingColor.x, specs.wingColor.y, specs.wingColor.z});
    // Left tip
    verts.insert(verts.end(), {-specs.wingspan/2.0f, 0.2f, -1.0f, specs.wingColor.x, specs.wingColor.y, specs.wingColor.z});
    // Right tip
    verts.insert(verts.end(), {specs.wingspan/2.0f, 0.2f, -1.0f, specs.wingColor.x, specs.wingColor.y, specs.wingColor.z});
    
    // Stabilizer (Triangle at back)
    verts.insert(verts.end(), {0.0f, 0.2f, -specs.fuselageLength/2.0f - 0.5f, specs.bodyColor.x, specs.bodyColor.y, specs.bodyColor.z});
    verts.insert(verts.end(), {-0.8f, 0.2f, -specs.fuselageLength/2.0f - 1.5f, specs.bodyColor.x, specs.bodyColor.y, specs.bodyColor.z});
    verts.insert(verts.end(), {0.8f, 0.2f, -specs.fuselageLength/2.0f - 1.5f, specs.bodyColor.x, specs.bodyColor.y, specs.bodyColor.z});
    
    return verts;
}

}
