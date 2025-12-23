#include "managers/asset_store.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace flightsim {

namespace {
    std::string readFile(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "Failed to open file: " << path << "\n";
            return "";
        }
        std::stringstream buffer;
        buffer << f.rdbuf();
        return buffer.str();
    }
}

bool AssetStore::loadShader(const std::string& name,
                            const std::string& vertPath,
                            const std::string& fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);

    if (vertSrc.empty() || fragSrc.empty()) return false;

    auto shader = std::make_unique<Shader>();
    if (!shader->init(vertSrc.c_str(), fragSrc.c_str())) {
        return false;
    }

    m_shaders[name] = std::move(shader);
    return true;
}

Shader* AssetStore::getShader(const std::string& name) {
    auto it = m_shaders.find(name);
    return it != m_shaders.end() ? it->second.get() : nullptr;
}

bool AssetStore::loadMesh(const std::string& name, const std::vector<float>& vertices) {
    auto mesh = std::make_unique<Mesh>();
    mesh->init(vertices);
    m_meshes[name] = std::move(mesh);
    return true;
}

Mesh* AssetStore::getMesh(const std::string& name) {
    auto it = m_meshes.find(name);
    return it != m_meshes.end() ? it->second.get() : nullptr;
}

void AssetStore::unloadAll() {
    m_shaders.clear();
    m_meshes.clear();
}

}