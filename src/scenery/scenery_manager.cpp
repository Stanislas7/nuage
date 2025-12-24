#include "scenery/scenery_manager.hpp"
#include "core/app.hpp"
#include "utils/config_loader.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader.hpp"
#include <iostream>

namespace nuage {

void SceneryManager::init(App* app) {
    m_app = app;
    m_shader = app->assets().getShader("basic");
}

void SceneryManager::loadConfig(const std::string& configPath) {
    auto jsonOpt = loadJsonConfig(configPath);
    if (!jsonOpt) return;
    const auto& json = *jsonOpt;

    if (json.contains("models")) {
        for (const auto& item : json["models"]) {
            std::string name = item.value("name", "");
            std::string path = item.value("path", "");
            if (!name.empty() && !path.empty()) {
                std::cout << "Loading model: " << name << " from " << path << std::endl;
                if (!m_app->assets().loadModel(name, path)) {
                    std::cerr << "Failed to load model: " << path << std::endl;
                }
            }
        }
    }

    if (json.contains("instances")) {
        for (const auto& item : json["instances"]) {
            SceneryObject obj;
            obj.meshName = item.value("model", "");
            
            if (item.contains("position")) {
                auto p = item["position"];
                if (p.is_array() && p.size() == 3) {
                    obj.transform.position = Vec3(p[0], p[1], p[2]);
                }
            }

            if (item.contains("scale")) {
                auto s = item["scale"];
                if (s.is_array() && s.size() == 3) {
                    obj.transform.scale = Vec3(s[0], s[1], s[2]);
                }
            }

            if (item.contains("rotation")) {
                auto r = item["rotation"];
                if (r.is_array() && r.size() == 3) {
                    float pitch = r[0]; // x
                    float yaw = r[1];   // y
                    float roll = r[2];  // z
                    
                    float toRad = 3.14159265f / 180.0f;
                    
                    Quat qy = Quat::fromAxisAngle(Vec3(0, 1, 0), yaw * toRad);
                    Quat qx = Quat::fromAxisAngle(Vec3(1, 0, 0), pitch * toRad);
                    Quat qz = Quat::fromAxisAngle(Vec3(0, 0, 1), roll * toRad);
                    
                    obj.transform.rotation = (qy * qx * qz).normalized();
                }
            }

            if (item.contains("color")) {
                auto c = item["color"];
                if (c.is_array() && c.size() == 3) {
                    obj.color = Vec3(c[0], c[1], c[2]);
                }
            }

            m_objects.push_back(obj);
        }
    }
}

void SceneryManager::render(const Mat4& viewProjection) {
    if (!m_shader) {
         m_shader = m_app->assets().getShader("basic");
         if (!m_shader) return;
    }

    m_shader->use();

    for (const auto& obj : m_objects) {
        Mesh* mesh = m_app->assets().getMesh(obj.meshName);
        if (mesh) {
            Mat4 mvp = viewProjection * obj.transform.matrix();
            m_shader->setMat4("uMVP", mvp);
            m_shader->setVec3("uColor", obj.color);
            m_shader->setBool("uUseUniformColor", true);
            mesh->draw();
        }
    }
    m_shader->setBool("uUseUniformColor", false);
}

}
