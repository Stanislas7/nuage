#include "core/simulator.hpp"
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";

    flightsim::Simulator sim;

    if (!sim.init({
        .windowWidth = 1280,
        .windowHeight = 720,
        .title = "Flight Simulator"
    })) {
        return -1;
    }

    // Load assets
    sim.assets().loadShader("basic", "assets/shaders/basic.vert",
                                     "assets/shaders/basic.frag");

    // Generate terrain
    sim.terrain().generate({.gridSize = 100, .scale = 10.0f});

    // Spawn player
    // Config path is dummy for now as we haven't implemented JSON loading fully
    sim.aircraft().spawnPlayer("assets/config/aircraft/cessna.json");

    sim.run();
    sim.shutdown();

    return 0;
}