#pragma once

#include <cstdint>
#include <string>

namespace flightsim {

// Entity identification
using EntityID = uint64_t;
constexpr EntityID INVALID_ENTITY = 0;

// Asset handles (type-safe references)
template<typename T>
struct Handle {
    uint32_t id = 0;
    bool valid() const { return id != 0; }
    bool operator==(const Handle& o) const { return id == o.id; }
};

class Shader;
class Mesh;

using ShaderHandle = Handle<Shader>;
using MeshHandle = Handle<Mesh>;

}
