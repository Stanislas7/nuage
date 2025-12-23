#pragma once

#include "graphics/mesh.hpp"
#include <vector>

namespace flightsim {

class Terrain {
public:
    void init(int gridSize, float scale);
    void draw() const;

private:
    std::vector<float> generateVertices(int gridSize, float scale);
    Mesh m_mesh;
};

}
