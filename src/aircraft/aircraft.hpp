#pragma once

#include "core/property_bus.hpp"
#include "aircraft/aircraft_component.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include <vector>
#include <memory>
#include <string>

namespace nuage {

class App;
class Mesh;
class Shader;
struct FlightInput;

class Aircraft {
public:
    void init(const std::string& configPath, App* app);
    void update(float dt, const FlightInput& input);
    void render(const Mat4& viewProjection);

    void setMesh(Mesh* mesh) { m_mesh = mesh; }
    void setShader(Shader* shader) { m_shader = shader; }
    Mesh* mesh() const { return m_mesh; }
    Shader* shader() const { return m_shader; }

    PropertyBus& state() { return m_state; }
    const PropertyBus& state() const { return m_state; }

    Vec3 position() const;
    Quat orientation() const;
    float airspeed() const;
    Vec3 forward() const;
    Vec3 up() const;
    Vec3 right() const;

    template<typename T, typename... Args>
    T* addSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        system->init(&m_state);
        m_systems.push_back(std::move(system));
        return ptr;
    }

    template<typename T>
    T* getSystem() {
        for (auto& system : m_systems) {
            T* typed = dynamic_cast<T*>(system.get());
            if (typed) return typed;
        }
        return nullptr;
    }

private:
    PropertyBus m_state;
    std::vector<std::unique_ptr<AircraftComponent>> m_systems;
    App* m_app = nullptr;
    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Vec3 m_color = {1, 1, 1};
};

}
