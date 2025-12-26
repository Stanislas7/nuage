#include "core/app.hpp"
#include "utils/config_loader.hpp"
#include <iostream>
#include <string>

int main() {
    auto configJson = nuage::loadJsonConfig("assets/config/simulator.json");
    
    nuage::AppConfig config;
    std::string windowTitle = "Nuage";

    if (configJson) {
        if (configJson->contains("window")) {
            const auto& win = (*configJson)["window"];
            config.windowWidth = win.value("width", 1280);
            config.windowHeight = win.value("height", 720);
            windowTitle = win.value("title", "Nuage");
            config.vsync = win.value("vsync", true);
        }
    }
    config.title = windowTitle.c_str();

    nuage::App app;

    if (!app.init(config)) {
        return -1;
    }

    app.run();
    app.shutdown();

    return 0;
}
