#pragma once

#include "graphics/shader.hpp"
#include "graphics/mesh.hpp"
#include "graphics/texture.hpp"
#include <unordered_map>
#include <memory>
#include <string>

namespace nuage {

class AssetStore {
public:
    // Shaders
    bool loadShader(const std::string& name,
                    const std::string& vertPath,
                    const std::string& fragPath);
    Shader* getShader(const std::string& name);

    // Meshes
    bool loadMesh(const std::string& name, const std::vector<float>& vertices);
    bool loadModel(const std::string& name, const std::string& path,
                   std::string* outDiffuseTexture = nullptr,
                   bool* outHasTexcoords = nullptr);
    Mesh* getMesh(const std::string& name);

    // Textures
    bool loadTexture(const std::string& name, const std::string& path);
    Texture* getTexture(const std::string& name);

    void unloadAll();

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
};

}
