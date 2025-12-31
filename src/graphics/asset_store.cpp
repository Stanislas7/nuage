#include "graphics/asset_store.hpp"
#include "utils/config_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "utils/tiny_obj_loader.h"

namespace nuage {

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

void AssetStore::init() {
    constexpr const char* kAssetConfigPath = "assets/config/assets.json";
    auto configOpt = loadJsonConfig(kAssetConfigPath);
    if (!configOpt || !configOpt->contains("shaders") || !(*configOpt)["shaders"].is_array()) {
        std::cerr << "Missing or invalid asset config: " << kAssetConfigPath << std::endl;
        throw std::runtime_error("AssetStore requires a valid assets/config/assets.json");
    }

    bool loadedAny = false;
    for (const auto& entry : (*configOpt)["shaders"]) {
        if (!entry.is_object()) continue;
        std::string name = entry.value("name", "");
        std::string vert = entry.value("vertex", "");
        std::string frag = entry.value("fragment", "");
        if (name.empty() || vert.empty() || frag.empty()) {
            std::cerr << "Invalid shader entry in " << kAssetConfigPath << std::endl;
            continue;
        }
        if (!loadShader(name, vert, frag)) {
            std::cerr << "Failed to load shader: " << name << std::endl;
        } else {
            loadedAny = true;
        }
    }

    if (!loadedAny) {
        throw std::runtime_error("AssetStore did not load any shaders from assets/config/assets.json");
    }
}

void AssetStore::update(double /*dt*/) {}

void AssetStore::shutdown() {
    unloadAll();
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

bool AssetStore::loadTexturedMesh(const std::string& name, const std::vector<float>& vertices) {
    auto mesh = std::make_unique<Mesh>();
    mesh->initTextured(vertices);
    m_meshes[name] = std::move(mesh);
    return true;
}

bool AssetStore::loadModel(const std::string& name, const std::string& path,
                           std::string* outDiffuseTexture,
                           bool* outHasTexcoords) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string baseDir = path.substr(0, path.find_last_of("/\\") + 1);

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str());

    if (!warn.empty()) {
        std::cout << "OBJ Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "OBJ Error: " << err << std::endl;
    }

    if (!ret) return false;

    std::string diffuseTexture;
    for (const auto& material : materials) {
        if (!material.diffuse_texname.empty()) {
            diffuseTexture = baseDir + material.diffuse_texname;
            break;
        }
    }
    if (outDiffuseTexture) {
        *outDiffuseTexture = diffuseTexture;
    }

    bool hasTexcoords = !attrib.texcoords.empty();
    if (outHasTexcoords) {
        *outHasTexcoords = hasTexcoords;
    }

    struct MaterialBuffers {
        std::vector<float> textured;
        std::vector<float> untextured;
    };

    std::unordered_map<int, MaterialBuffers> materialVertices;
    std::vector<int> materialOrder;

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shape.mesh.num_face_vertices[f]);
            int materialId = -1;
            if (f < shape.mesh.material_ids.size()) {
                materialId = shape.mesh.material_ids[f];
            }
            if (materialVertices.find(materialId) == materialVertices.end()) {
                materialOrder.push_back(materialId);
            }
            auto& buffers = materialVertices[materialId];

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                tinyobj::real_t nx = 0.0f;
                tinyobj::real_t ny = 1.0f;
                tinyobj::real_t nz = 0.0f;
                if (idx.normal_index >= 0) {
                    nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                buffers.untextured.push_back(static_cast<float>(vx));
                buffers.untextured.push_back(static_cast<float>(vy));
                buffers.untextured.push_back(static_cast<float>(vz));
                buffers.untextured.push_back(static_cast<float>(nx));
                buffers.untextured.push_back(static_cast<float>(ny));
                buffers.untextured.push_back(static_cast<float>(nz));
                buffers.untextured.push_back(1.0f);
                buffers.untextured.push_back(1.0f);
                buffers.untextured.push_back(1.0f);

                if (hasTexcoords) {
                    tinyobj::real_t tu = 0.0f;
                    tinyobj::real_t tv = 0.0f;
                    if (idx.texcoord_index >= 0) {
                        tu = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                        tv = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    }
                    buffers.textured.push_back(static_cast<float>(vx));
                    buffers.textured.push_back(static_cast<float>(vy));
                    buffers.textured.push_back(static_cast<float>(vz));
                    buffers.textured.push_back(static_cast<float>(nx));
                    buffers.textured.push_back(static_cast<float>(ny));
                    buffers.textured.push_back(static_cast<float>(nz));
                    buffers.textured.push_back(static_cast<float>(tu));
                    buffers.textured.push_back(static_cast<float>(tv));
                }
            }
            index_offset += fv;
        }
    }

    auto model = std::make_unique<Model>();
    size_t partIndex = 0;
    for (int materialId : materialOrder) {
        auto it = materialVertices.find(materialId);
        if (it == materialVertices.end()) {
            continue;
        }

        std::string meshName = (partIndex == 0) ? name : (name + "::part" + std::to_string(partIndex));
        partIndex++;

        bool hasDiffuse = false;
        if (materialId >= 0 && materialId < static_cast<int>(materials.size())) {
            hasDiffuse = !materials[materialId].diffuse_texname.empty();
        }

        auto mesh = std::make_unique<Mesh>();
        bool useTextured = hasTexcoords && hasDiffuse && !it->second.textured.empty();
        if (useTextured) {
            mesh->initTextured(it->second.textured);
        } else {
            if (!it->second.untextured.empty()) {
                mesh->init(it->second.untextured);
            } else {
                continue;
            }
        }

        Mesh* meshPtr = mesh.get();
        m_meshes[meshName] = std::move(mesh);

        Texture* texture = nullptr;
        bool textured = false;
        if (useTextured && materialId >= 0 && materialId < static_cast<int>(materials.size())) {
            const auto& material = materials[materialId];
            std::string texturePath = baseDir + material.diffuse_texname;
            std::string textureName = name + "_mat_" + std::to_string(materialId);
            if (loadTexture(textureName, texturePath)) {
                texture = getTexture(textureName);
                textured = true;
            }
        }

        model->addPart(meshPtr, texture, textured);
    }

    m_models[name] = std::move(model);

    if (!hasTexcoords) {
        return true;
    }

    return true;
}

Mesh* AssetStore::getMesh(const std::string& name) {
    auto it = m_meshes.find(name);
    return it != m_meshes.end() ? it->second.get() : nullptr;
}

Model* AssetStore::getModel(const std::string& name) {
    auto it = m_models.find(name);
    return it != m_models.end() ? it->second.get() : nullptr;
}

bool AssetStore::loadTexture(const std::string& name, const std::string& path, bool repeat) {
    auto texture = std::make_unique<Texture>();
    if (!texture->loadFromFile(path, true, repeat)) {
        return false;
    }
    m_textures[name] = std::move(texture);
    return true;
}

Texture* AssetStore::getTexture(const std::string& name) {
    auto it = m_textures.find(name);
    return it != m_textures.end() ? it->second.get() : nullptr;
}

void AssetStore::unloadAll() {
    m_shaders.clear();
    m_meshes.clear();
    m_textures.clear();
    m_models.clear();
}

}
