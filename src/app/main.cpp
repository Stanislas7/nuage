#include "app/App.hpp"
#include "utils/ConfigLoader.hpp"
#include <iostream>
#include <filesystem>
#include <string>

int main() {
    std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";

    auto configJson = nuage::loadJsonConfig("assets/config/simulator.json");
    
    nuage::AppConfig config;
    std::string windowTitle = "Nuage";

    std::string aircraftPath = "assets/config/aircraft/cessna.json";

    if (configJson) {
        if (configJson->contains("window")) {
            const auto& win = (*configJson)["window"];
            config.windowWidth = win.value("width", 1280);
            config.windowHeight = win.value("height", 720);
            windowTitle = win.value("title", "Nuage");
            config.vsync = win.value("vsync", true);
        }
        if (configJson->contains("simulation")) {
            aircraftPath = (*configJson)["simulation"].value("defaultAircraft", aircraftPath);
        }
    }
    config.title = windowTitle.c_str();

    nuage::App app;

    if (!app.init(config)) {
        return -1;
    }

    auto* player = app.aircraft().spawnPlayer(aircraftPath);
    if (player) {
        player->setMesh(app.assets().getMesh("aircraft"));
        player->setShader(app.assets().getShader("basic"));
    }

    app.run();
    app.shutdown();

    return 0;
}
