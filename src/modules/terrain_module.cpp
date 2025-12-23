#include "modules/terrain_module.hpp"
#include "core/simulator.hpp"
#include <glad/glad.h>
#include <vector>
#include <cmath>

namespace flightsim {

void TerrainModule::init(Simulator* sim) {
    Module::init(sim);
    generate(TerrainParams{});
}

void TerrainModule::generate(const TerrainParams& params) {
    m_params = params;
    
    // Simple grid generation
    std::vector<float> verts;
    float halfSize = params.gridSize * params.scale / 2.0f;
    
    // Generate a simple plane for now
    // Quad 1
    verts.insert(verts.end(), {-halfSize, 0, -halfSize, 0.2f, 0.8f, 0.2f});
    verts.insert(verts.end(), {-halfSize, 0, halfSize, 0.2f, 0.8f, 0.2f});
    verts.insert(verts.end(), {halfSize, 0, halfSize, 0.2f, 0.8f, 0.2f});
    
    // Quad 2
    verts.insert(verts.end(), {-halfSize, 0, -halfSize, 0.2f, 0.8f, 0.2f});
    verts.insert(verts.end(), {halfSize, 0, halfSize, 0.2f, 0.8f, 0.2f});
    verts.insert(verts.end(), {halfSize, 0, -halfSize, 0.2f, 0.8f, 0.2f});
    
    m_mesh.init(verts);
    m_generated = true;
}

void TerrainModule::render() {
    if (!m_generated) return;
    
    auto* shader = m_sim->assets().getShader("basic");
    if (!shader) return;

    shader->use();
    GLint mvpLoc = shader->getUniformLocation("uMVP");

    const auto& ctx = m_sim->camera().renderContext();
    Mat4 mvp = ctx.viewProjection; // Identity model matrix for terrain
    
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp.data());
    
    m_mesh.draw();
}

float TerrainModule::heightAt(float x, float z) const {
    return 0.0f; // Flat terrain for now
}

void TerrainModule::shutdown() {
    // Mesh cleans itself up
}

}
