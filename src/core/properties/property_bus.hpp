#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"
#include "core/properties/property_id.hpp"
#include <string>
#include <unordered_map>
#include <variant>
#include <optional>

namespace nuage {

using PropertyValue = std::variant<double, Vec3, Quat, int, bool>;

class PropertyBus {
public:
    static PropertyId getID(const std::string& key);

    template<typename T>
    void set(PropertyId id, const T& value) {
        m_data[id] = value;
    }

    template<typename T>
    T get(PropertyId id, const T& fallback = T()) const {
        auto it = m_data.find(id);
        if (it != m_data.end() && std::holds_alternative<T>(it->second)) {
            return std::get<T>(it->second);
        }
        return fallback;
    }

    template<typename T>
    void set(const TypedProperty<T>& prop, const T& value) {
        set<T>(prop.id, value);
    }

    template<typename T>
    T get(const TypedProperty<T>& prop, const T& fallback = T()) const {
        return get<T>(prop.id, fallback);
    }

    template<typename T>
    void set(const std::string& key, const T& value) {
        set<T>(getID(key), value);
    }

    template<typename T>
    T get(const std::string& key, const T& fallback = T()) const {
        return get<T>(getID(key), fallback);
    }

    bool has(PropertyId id) const;
    bool has(const std::string& key) const;

    void increment(PropertyId id, double delta);
    void increment(const std::string& key, double delta);

    static PropertyBus& global() {
        static PropertyBus instance;
        return instance;
    }
    
private:
    std::unordered_map<PropertyId, PropertyValue> m_data;
};

}
