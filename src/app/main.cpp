#include "app/simulator.hpp"
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

    sim.assets().loadShader("basic", "assets/shaders/basic.vert",
                                     "assets/shaders/basic.frag");

    sim.terrain().generate({.gridSize = 100, .scale = 10.0f});

    sim.aircraft().spawnPlayer("assets/config/aircraft/cessna.json");

    sim.run();
    sim.shutdown();

    return 0;
}