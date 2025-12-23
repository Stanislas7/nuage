#include "graphics/mesh_builder.hpp"

namespace nuage {

std::vector<float> MeshBuilder::aircraft(const AircraftMeshSpecs& specs) {
    std::vector<float> verts;
    float L = specs.fuselageLength;
    float W = specs.wingspan;
    Vec3 bc = specs.bodyColor;
    Vec3 wc = specs.wingColor;
    Vec3 cockpit = {0.3f, 0.5f, 0.7f};  // Blue cockpit
    Vec3 accent = {0.9f, 0.9f, 0.9f};   // White accents

    auto tri = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 col) {
        verts.insert(verts.end(), {a.x, a.y, a.z, col.x, col.y, col.z});
        verts.insert(verts.end(), {b.x, b.y, b.z, col.x, col.y, col.z});
        verts.insert(verts.end(), {c.x, c.y, c.z, col.x, col.y, col.z});
    };

    // Fuselage - tapered body (top)
    tri({0, 0.15f, L*0.5f}, {-0.3f, 0.3f, -L*0.3f}, {0.3f, 0.3f, -L*0.3f}, bc);
    // Fuselage - bottom
    tri({0, -0.1f, L*0.5f}, {0.3f, -0.15f, -L*0.3f}, {-0.3f, -0.15f, -L*0.3f}, bc);
    // Fuselage - left side
    tri({0, 0.15f, L*0.5f}, {-0.3f, -0.15f, -L*0.3f}, {-0.3f, 0.3f, -L*0.3f}, {bc.x*0.8f, bc.y*0.8f, bc.z*0.8f});
    tri({0, 0.15f, L*0.5f}, {0, -0.1f, L*0.5f}, {-0.3f, -0.15f, -L*0.3f}, {bc.x*0.8f, bc.y*0.8f, bc.z*0.8f});
    // Fuselage - right side
    tri({0, 0.15f, L*0.5f}, {0.3f, 0.3f, -L*0.3f}, {0.3f, -0.15f, -L*0.3f}, {bc.x*0.9f, bc.y*0.9f, bc.z*0.9f});
    tri({0, 0.15f, L*0.5f}, {0.3f, -0.15f, -L*0.3f}, {0, -0.1f, L*0.5f}, {bc.x*0.9f, bc.y*0.9f, bc.z*0.9f});

    // Tail section
    tri({-0.3f, 0.3f, -L*0.3f}, {0, 0.1f, -L*0.5f}, {0.3f, 0.3f, -L*0.3f}, bc);
    tri({-0.3f, -0.15f, -L*0.3f}, {0.3f, -0.15f, -L*0.3f}, {0, -0.05f, -L*0.5f}, bc);

    // Cockpit (canopy)
    tri({0, 0.35f, L*0.2f}, {-0.2f, 0.3f, -0.2f}, {0.2f, 0.3f, -0.2f}, cockpit);

    // Main wings - left
    tri({0, 0.1f, 0.3f}, {-W*0.5f, 0.15f, -0.3f}, {0, 0.1f, -0.8f}, wc);
    tri({0, 0.05f, 0.3f}, {0, 0.05f, -0.8f}, {-W*0.5f, 0.1f, -0.3f}, {wc.x*0.85f, wc.y*0.85f, wc.z*0.85f});
    // Main wings - right
    tri({0, 0.1f, 0.3f}, {0, 0.1f, -0.8f}, {W*0.5f, 0.15f, -0.3f}, wc);
    tri({0, 0.05f, 0.3f}, {W*0.5f, 0.1f, -0.3f}, {0, 0.05f, -0.8f}, {wc.x*0.85f, wc.y*0.85f, wc.z*0.85f});

    // Horizontal stabilizer
    tri({0, 0.1f, -L*0.45f}, {-1.0f, 0.12f, -L*0.55f}, {0, 0.1f, -L*0.55f}, wc);
    tri({0, 0.1f, -L*0.45f}, {0, 0.1f, -L*0.55f}, {1.0f, 0.12f, -L*0.55f}, wc);

    // Vertical stabilizer (tail fin)
    tri({0, 0.1f, -L*0.4f}, {0, 0.7f, -L*0.55f}, {0, 0.1f, -L*0.55f}, {bc.x*0.7f, bc.y*0.7f, bc.z*0.7f});

    // Engine cowling accent (white stripe)
    tri({0, 0.2f, L*0.35f}, {-0.15f, 0.25f, L*0.2f}, {0.15f, 0.25f, L*0.2f}, accent);

    return verts;
}

std::vector<float> MeshBuilder::terrain(float size, int subdivisions) {
    std::vector<float> verts;
    float halfSize = size / 2.0f;
    float step = size / static_cast<float>(subdivisions);
    
    for (int i = 0; i < subdivisions; ++i) {
        for (int j = 0; j < subdivisions; ++j) {
            float x0 = -halfSize + i * step;
            float z0 = -halfSize + j * step;
            float x1 = x0 + step;
            float z1 = z0 + step;
            
            float g = 0.25f + 0.1f * ((i + j) % 2);
            float r = g * 0.5f;
            float b = g * 0.4f;
            
            verts.insert(verts.end(), {x0, 0.0f, z0, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z0, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z1, r, g, b});
            
            verts.insert(verts.end(), {x0, 0.0f, z0, r, g, b});
            verts.insert(verts.end(), {x1, 0.0f, z1, r, g, b});
            verts.insert(verts.end(), {x0, 0.0f, z1, r, g, b});
        }
    }
    
    return verts;
}

}
