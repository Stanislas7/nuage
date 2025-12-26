#pragma once

#include <vector>

namespace nuage {

class Mesh;
class Texture;

struct ModelPart {
    Mesh* mesh = nullptr;
    Texture* texture = nullptr;
    bool textured = false;
};

class Model {
public:
    void addPart(Mesh* mesh, Texture* texture, bool textured) {
        ModelPart part;
        part.mesh = mesh;
        part.texture = texture;
        part.textured = textured;
        m_parts.push_back(part);
    }

    const std::vector<ModelPart>& parts() const { return m_parts; }

private:
    std::vector<ModelPart> m_parts;
};

}
