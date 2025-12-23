#include "app/App.hpp"
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";

    nuage::App app;

    if (!app.init({
        .windowWidth = 1280,
        .windowHeight = 720,
        .title = "Nuage"
    })) {
        return -1;
    }

    app.aircraft().spawnPlayer("assets/config/aircraft/cessna.json");

    app.run();
    app.shutdown();

    return 0;
}
