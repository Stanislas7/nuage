#pragma once

#include "core/property_bus.hpp"
#include "aircraft/aircraft_component.hpp"
#include "aircraft/aircraft_state.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "math/mat4.hpp"
#include <vector>
#include <memory>
#include <string>

namespace nuage {

class AssetStore;
class Atmosphere;
class Mesh;
class Shader;
class Texture;
class Model;
class Input;
struct FlightInput;

class Aircraft {
public:
    class Instance {
    public:
        void init(const std::string& configPath, AssetStore& assets, Atmosphere& atmosphere);
        void update(float dt, const FlightInput& input);
        void render(const Mat4& viewProjection, float alpha = 1.0f);

        void setMesh(Mesh* mesh) { m_mesh = mesh; }
        void setShader(Shader* shader) { m_shader = shader; }
        Mesh* mesh() const { return m_mesh; }
        Shader* shader() const { return m_shader; }

        PropertyBus& state() { return m_state; }
        const PropertyBus& state() const { return m_state; }

        Vec3 position() const;
        Quat orientation() const;
        
        // Interpolated getters for rendering
        Vec3 interpolatedPosition(float alpha) const;
        Quat interpolatedOrientation(float alpha) const;

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
        void updateCacheFromBus();

        PropertyBus m_state;
        AircraftState m_currentState;
        AircraftState m_prevState;
        
        std::vector<std::unique_ptr<AircraftComponent>> m_systems;
        Mesh* m_mesh = nullptr;
        Shader* m_shader = nullptr;
        Texture* m_texture = nullptr;
        Model* m_model = nullptr;
        Shader* m_texturedShader = nullptr;
        Vec3 m_color = {1, 1, 1};
        Vec3 m_modelScale = {1, 1, 1};
        Quat m_modelRotation = Quat::identity();
        Vec3 m_modelOffset = {0, 0, 0};
    };

    void init(AssetStore& assets, Atmosphere& atmosphere);
    void fixedUpdate(float dt, const Input& input);
    void render(const Mat4& viewProjection, float alpha = 1.0f);
    void shutdown();

    Instance* spawnPlayer(const std::string& configPath);

    Instance* player() { return m_player; }
    const std::vector<std::unique_ptr<Instance>>& all() const { return m_instances; }

    void destroy(Instance* aircraft);
    void destroyAll();

private:
    AssetStore* m_assets = nullptr;
    Atmosphere* m_atmosphere = nullptr;
    std::vector<std::unique_ptr<Instance>> m_instances;
    Instance* m_player = nullptr;
};

}
