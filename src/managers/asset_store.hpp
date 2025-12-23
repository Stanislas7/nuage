#pragma once

#include "app/types.hpp"
#include "graphics/shader.hpp"
#include "graphics/mesh.hpp"
#include <unordered_map>
#include <memory>
#include <string>

namespace flightsim {

class AssetStore {
public:
    // Shaders
    bool loadShader(const std::string& name,
                    const std::string& vertPath,
                    const std::string& fragPath);
    Shader* getShader(const std::string& name);

    // Meshes
    bool loadMesh(const std::string& name, const std::vector<float>& vertices);
    Mesh* getMesh(const std::string& name);

    void unloadAll();

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
};

}
