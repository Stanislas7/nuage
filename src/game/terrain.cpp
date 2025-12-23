#include "game/terrain.hpp"
#include "math/vec3.hpp"
#include <cmath>

namespace flightsim {

void Terrain::init(int gridSize, float scale) {
    m_mesh.init(generateVertices(gridSize, scale));
}

void Terrain::draw() const {
    m_mesh.draw();
}

std::vector<float> Terrain::generateVertices(int gridSize, float scale) {
    std::vector<float> verts;

    for (int z = 0; z < gridSize; z++) {
        for (int x = 0; x < gridSize; x++) {
            float x0 = (x - gridSize / 2.0f) * scale;
            float z0 = (z - gridSize / 2.0f) * scale;
            float x1 = x0 + scale;
            float z1 = z0 + scale;

            auto height = [](float px, float pz) {
                return std::sin(px * 0.05f) * std::cos(pz * 0.05f) * 5.0f +
                       std::sin(px * 0.02f + pz * 0.03f) * 8.0f;
            };

            float y00 = height(x0, z0);
            float y10 = height(x1, z0);
            float y01 = height(x0, z1);
            float y11 = height(x1, z1);

            auto color = [](float h) -> Vec3 {
                if (h < -2) return Vec3(0.2f, 0.4f, 0.8f);
                if (h < 2) return Vec3(0.3f, 0.6f, 0.2f);
                if (h < 6) return Vec3(0.5f, 0.5f, 0.3f);
                return Vec3(0.9f, 0.9f, 0.95f);
            };

            Vec3 c00 = color(y00), c10 = color(y10), c01 = color(y01), c11 = color(y11);

            verts.insert(verts.end(), {x0, y00, z0, c00.x, c00.y, c00.z});
            verts.insert(verts.end(), {x1, y10, z0, c10.x, c10.y, c10.z});
            verts.insert(verts.end(), {x0, y01, z1, c01.x, c01.y, c01.z});
            verts.insert(verts.end(), {x1, y10, z0, c10.x, c10.y, c10.z});
            verts.insert(verts.end(), {x1, y11, z1, c11.x, c11.y, c11.z});
            verts.insert(verts.end(), {x0, y01, z1, c01.x, c01.y, c01.z});
        }
    }
    return verts;
}

}
