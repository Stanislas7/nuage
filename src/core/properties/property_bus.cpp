#include "core/properties/property_bus.hpp"

namespace nuage {

PropertyId PropertyBus::getID(const std::string& key) {
    return hashString(key.c_str(), key.length());
}

bool PropertyBus::has(PropertyId id) const {
    return m_data.find(id) != m_data.end();
}

bool PropertyBus::has(const std::string& key) const {
    return has(getID(key));
}

void PropertyBus::increment(PropertyId id, double delta) {
    auto it = m_data.find(id);
    if (it != m_data.end() && std::holds_alternative<double>(it->second)) {
        std::get<double>(it->second) += delta;
    } else {
        m_data[id] = delta;
    }
}

void PropertyBus::increment(const std::string& key, double delta) {
    increment(getID(key), delta);
}

}