#pragma once

#include "core/module.hpp"
#include "graphics/mesh.hpp"

namespace flightsim {

struct TerrainParams {
    int gridSize = 100;
    float scale = 10.0f;
    float heightScale = 1.0f;
};

class TerrainModule : public Module {
public:
    const char* name() const override { return "Terrain"; }

    void init(Simulator* sim) override;
    void update(float dt) override {}; // Override default empty implementation explicitly if desired
    void render() override;
    void shutdown() override;

    // Configuration
    void generate(const TerrainParams& params);

    // Height queries (for physics/collision)
    float heightAt(float x, float z) const;

private:
    Mesh m_mesh;
    TerrainParams m_params;
    bool m_generated = false;
};

}
