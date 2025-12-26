#include "core/property_bus.hpp"

namespace nuage {

PropertyId PropertyBus::getID(const std::string& key) {
    return hashString(key.c_str(), key.length());
}

void PropertyBus::set(PropertyId id, double value) {
    m_data[id] = value;
}

double PropertyBus::get(PropertyId id, double fallback) const {
    auto it = m_data.find(id);
    if (it != m_data.end() && std::holds_alternative<double>(it->second)) {
        return std::get<double>(it->second);
    }
    return fallback;
}

bool PropertyBus::has(PropertyId id) const {
    return m_data.find(id) != m_data.end();
}

void PropertyBus::set(const std::string& key, double value) {
    set(getID(key), value);
}

double PropertyBus::get(const std::string& key, double fallback) const {
    return get(getID(key), fallback);
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