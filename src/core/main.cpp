#include "core/app.hpp"
#include "utils/config_loader.hpp"
#include <iostream>
#include <filesystem>
#include <string>

int main() {
    auto configJson = nuage::loadJsonConfig("assets/config/simulator.json");
    
    nuage::AppConfig config;
    std::string windowTitle = "Nuage";
    std::string aircraftPath = "assets/config/aircraft/c172p.json";

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

    // Prepare and start the initial flight
    nuage::FlightConfig flight;
    flight.aircraftPath = aircraftPath;
    flight.terrainPath = "assets/config/terrain.json";
    flight.sceneryPath = "assets/config/scenery.json";
    flight.timeOfDay = 12.0f;

    if (!app.startFlight(flight)) {
        std::cerr << "Failed to start initial flight session" << std::endl;
        return -1;
    }

    app.run();
    app.shutdown();

    return 0;
}