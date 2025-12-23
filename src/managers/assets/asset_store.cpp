#include "managers/assets/asset_store.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

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

bool AssetStore::loadModel(const std::string& name, const std::string& path) {
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

    std::vector<float> vertices;

    // Iterate over shapes
    for (const auto& shape : shapes) {
        // Iterate over faces
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shape.mesh.num_face_vertices[f]);

            // Triangulate if necessary (tinyobjloader usually handles this if requested, but let's assume triangles)
            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
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

                vertices.push_back(static_cast<float>(vx));
                vertices.push_back(static_cast<float>(vy));
                vertices.push_back(static_cast<float>(vz));
                vertices.push_back(static_cast<float>(nx));
                vertices.push_back(static_cast<float>(ny));
                vertices.push_back(static_cast<float>(nz));
            }
            index_offset += fv;
        }
    }

    return loadMesh(name, vertices);
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