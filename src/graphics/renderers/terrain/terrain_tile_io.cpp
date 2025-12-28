#include "graphics/renderers/terrain/terrain_tile_io.hpp"
#include <fstream>

namespace nuage {

bool load_compiled_mesh(const std::string& path, std::vector<float>& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    char magic[4] = {};
    in.read(magic, 4);
    if (in.gcount() != 4 || std::string(magic, 4) != "NTM1") {
        return false;
    }
    std::uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in || count == 0) {
        return false;
    }
    out.resize(count);
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(count * sizeof(float)));
    return static_cast<std::size_t>(in.gcount()) == count * sizeof(float);
}

bool load_compiled_mask(const std::string& path, int expectedRes, std::vector<std::uint8_t>& out) {
    if (expectedRes <= 0) {
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    size_t size = static_cast<size_t>(expectedRes * expectedRes);
    out.resize(size);
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(in.gcount()) == size;
}

} // namespace nuage
