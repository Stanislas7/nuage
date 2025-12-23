#include "managers/aircraft_module.hpp"
#include "app/simulator.hpp"
#include "graphics/mesh_builder.hpp"
#include "graphics/glad.h"

namespace flightsim {

void AircraftModule::init(Simulator* sim) {
    Module::init(sim);
    initDefaultMesh();
}

void AircraftModule::initDefaultMesh() {
    // Simple aircraft shape
    auto verts = MeshBuilder::aircraft({
        .fuselageLength = 4.0f,
        .wingspan = 6.0f,
        .bodyColor = {0.8f, 0.2f, 0.2f},
        .wingColor = {0.3f, 0.3f, 0.4f}
    });
    m_defaultMesh.init(verts);
}

Aircraft& AircraftModule::spawn(const std::string& configPath) {
    auto aircraft = std::make_unique<Aircraft>();
    aircraft->id = m_nextId++;
    aircraft->model.init(FlightParams::load(configPath));

    Aircraft& ref = *aircraft;
    m_aircraft.push_back(std::move(aircraft));
    return ref;
}

Aircraft& AircraftModule::spawnPlayer(const std::string& configPath) {
    Aircraft& a = spawn(configPath);
    a.isPlayer = true;
    a.name = "Player";
    m_player = &a;
    return a;
}

void AircraftModule::update(float dt) {
    const auto& controls = m_sim->input().controls();

    for (auto& aircraft : m_aircraft) {
        if (aircraft->isPlayer) {
            // Player uses input controls
            aircraft->model.update(dt, controls);
        } else {
            // AI aircraft - no controls for now
            aircraft->model.update(dt, FlightControls{});
        }
    }
}

void AircraftModule::render() {
    auto* shader = m_sim->assets().getShader("basic");
    if (!shader) return;

    shader->use();
    GLint mvpLoc = shader->getUniformLocation("uMVP");

    const auto& ctx = m_sim->camera().renderContext();

    for (const auto& aircraft : m_aircraft) {
        Mat4 model = Mat4::translate(aircraft->model.position())
                   * aircraft->model.orientation().toMat4()
                   * Mat4::scale(2.0f, 2.0f, 2.0f);

        Mat4 mvp = ctx.viewProjection * model;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp.data());

        m_defaultMesh.draw();
    }
}

void AircraftModule::shutdown() {
    destroyAll();
}

void AircraftModule::destroyAll() {
    m_aircraft.clear();
    m_player = nullptr;
}

Aircraft* AircraftModule::find(EntityID id) {
    for (auto& a : m_aircraft) {
        if (a->id == id) return a.get();
    }
    return nullptr;
}

}
