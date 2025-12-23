#pragma once

#include "world/transform.hpp"
#include <vector>
#include <string>

namespace nuage {

class App;
class Shader;

struct SceneryObject {
    std::string meshName;
    Transform transform;
    Vec3 color{1, 1, 1};
};

class SceneryManager {
public:
    void init(App* app);
    void loadConfig(const std::string& configPath);
    void render(const Mat4& viewProjection);

private:
    App* m_app = nullptr;
    std::vector<SceneryObject> m_objects;
    Shader* m_shader = nullptr;
};

}
