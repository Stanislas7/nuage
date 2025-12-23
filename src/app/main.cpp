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

    auto* player = app.aircraft().spawnPlayer("assets/config/aircraft/cessna.json");
    if (player) {
        player->setMesh(app.assets().getMesh("aircraft"));
        player->setShader(app.assets().getShader("basic"));
    }

    app.run();
    app.shutdown();

    return 0;
}
