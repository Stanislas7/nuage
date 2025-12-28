#pragma once

#include "core/subsystem.hpp"
#include "graphics/shader.hpp"
#include "graphics/mesh.hpp"
#include "graphics/texture.hpp"
#include "graphics/model.hpp"
#include <unordered_map>
#include <memory>
#include <string>

namespace nuage {

class AssetStore : public Subsystem {
public:
    // Subsystem interface
    void init() override;
    void update(double dt) override;
    void shutdown() override;
    std::string getName() const override { return "AssetStore"; }

    // Shaders
    bool loadShader(const std::string& name,
                    const std::string& vertPath,
                    const std::string& fragPath);
    Shader* getShader(const std::string& name);

    // Meshes
    bool loadMesh(const std::string& name, const std::vector<float>& vertices);
    bool loadTexturedMesh(const std::string& name, const std::vector<float>& vertices);
    bool loadModel(const std::string& name, const std::string& path,
                   std::string* outDiffuseTexture = nullptr,
                   bool* outHasTexcoords = nullptr);
    Mesh* getMesh(const std::string& name);
    Model* getModel(const std::string& name);

    // Textures
    bool loadTexture(const std::string& name, const std::string& path);
    Texture* getTexture(const std::string& name);

    void unloadAll();

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
    std::unordered_map<std::string, std::unique_ptr<Model>> m_models;
};

}
