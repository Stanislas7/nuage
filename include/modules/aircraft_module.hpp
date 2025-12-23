#pragma once

#include "core/module.hpp"
#include "core/types.hpp"
#include "simulation/flight_model.hpp"
#include "graphics/mesh.hpp"
#include <vector>
#include <memory>

namespace flightsim {

// An aircraft instance
struct Aircraft {
    EntityID id = INVALID_ENTITY;
    std::string name;
    FlightModel model;
    MeshHandle mesh;
    ShaderHandle shader;
    bool isPlayer = false;
};

class AircraftModule : public Module {
public:
    const char* name() const override { return "Aircraft"; }

    void init(Simulator* sim) override;
    void update(float dt) override;
    void render() override;
    void shutdown() override;

    // Spawn aircraft
    Aircraft& spawn(const std::string& configPath);
    Aircraft& spawnPlayer(const std::string& configPath);

    // Access
    Aircraft* player() { return m_player; }
    Aircraft* find(EntityID id);
    const std::vector<std::unique_ptr<Aircraft>>& all() const { return m_aircraft; }

    // Destroy
    void destroy(EntityID id);
    void destroyAll();

private:
    void initDefaultMesh();

    std::vector<std::unique_ptr<Aircraft>> m_aircraft;
    Aircraft* m_player = nullptr;
    EntityID m_nextId = 1;

    // Default mesh for aircraft
    Mesh m_defaultMesh;
};

}
