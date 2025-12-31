#pragma once

#include "core/properties/property_bus.hpp"
#include "aircraft/aircraft_component.hpp"
#include "aircraft/aircraft_state.hpp"
#include "aircraft/aircraft_visual.hpp"
#include "core/properties/property_context.hpp"
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

class Aircraft {
public:
    class Instance {
    public:
        void init(const std::string& configPath, AssetStore& assets, Atmosphere& atmosphere);
        void update(float dt);
        void render(const Mat4& viewProjection, float alpha, const Vec3& lightDir);

        PropertyBus& state() { return m_state; }
        const PropertyBus& state() const { return m_state; }

        // Interpolated getters for rendering
        Vec3 interpolatedPosition(float alpha) const;
        Quat interpolatedOrientation(float alpha) const;

        template<typename T, typename... Args>
        T* addSystem(Args&&... args) {
            auto system = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = system.get();
            system->init(m_currentState, m_properties);
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
        PropertyContext m_properties;
        AircraftState m_currentState;
        AircraftState m_prevState;
        
        std::vector<std::unique_ptr<AircraftComponent>> m_systems;
        AircraftVisual m_visual;
    };

    void init(AssetStore& assets, Atmosphere& atmosphere);
    void fixedUpdate(float dt);
    void render(const Mat4& viewProjection, float alpha, const Vec3& lightDir);
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
