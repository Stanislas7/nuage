#pragma once

#include "math/transform.hpp"
#include <vector>
#include <string>

namespace nuage {

class AssetStore;
class Shader;

struct SceneryObject {
    std::string meshName;
    Transform transform;
    Vec3 color{1, 1, 1};
};

class Scenery {
public:
    void init(AssetStore& assets);
    void loadConfig(const std::string& configPath);
    void render(const Mat4& viewProjection);

private:
    AssetStore* m_assets = nullptr;
    std::vector<SceneryObject> m_objects;
    Shader* m_shader = nullptr;
};

}