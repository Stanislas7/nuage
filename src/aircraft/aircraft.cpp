#include "aircraft/aircraft.hpp"
#include "input/input.hpp"
#include <algorithm>

namespace nuage {

void Aircraft::init(AssetStore& assets, Atmosphere& atmosphere) {
    m_assets = &assets;
    m_atmosphere = &atmosphere;
}

void Aircraft::fixedUpdate(float dt) {
    for (auto& ac : m_instances) {
        ac->update(dt);
    }
}

void Aircraft::applyGroundCollision(const TerrainRenderer& terrain) {
    for (auto& ac : m_instances) {
        ac->applyGroundCollision(terrain);
    }
}

void Aircraft::render(const Mat4& viewProjection, float alpha, const Vec3& lightDir) {
    for (auto& ac : m_instances) {
        ac->render(viewProjection, alpha, lightDir);
    }
}

void Aircraft::shutdown() {
    destroyAll();
}

Aircraft::Instance* Aircraft::spawnPlayer(const std::string& configPath,
                                          const GeoOrigin* terrainOrigin,
                                          const TerrainRenderer* terrain) {
    if (!m_assets || !m_atmosphere) return nullptr;

    auto aircraft = std::make_unique<Aircraft::Instance>();
    aircraft->init(configPath, *m_assets, *m_atmosphere, terrainOrigin, terrain);
    
    m_player = aircraft.get();
    m_instances.push_back(std::move(aircraft));
    return m_player;
}

void Aircraft::destroy(Instance* aircraft) {
    auto it = std::find_if(m_instances.begin(), m_instances.end(),
        [aircraft](const std::unique_ptr<Instance>& ac) { return ac.get() == aircraft; });
    
    if (it != m_instances.end()) {
        if (m_player == aircraft) {
            m_player = nullptr;
        }
        m_instances.erase(it);
    }
}

void Aircraft::destroyAll() {
    m_instances.clear();
    m_player = nullptr;
}

}
