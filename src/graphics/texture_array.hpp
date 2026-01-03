#pragma once

#include "graphics/glad.h"
#include <string>
#include <vector>

namespace nuage {

class TextureArray {
public:
    TextureArray() = default;
    ~TextureArray();

    bool loadFromFiles(const std::vector<std::string>& paths, int targetSize, bool repeat = true);
    void bind(GLuint unit = 0) const;
    GLuint id() const { return m_id; }
    int layers() const { return m_layers; }
    int size() const { return m_size; }

private:
    GLuint m_id = 0;
    int m_layers = 0;
    int m_size = 0;
};

} // namespace nuage
