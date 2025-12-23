#include "aircraft/property_bus.hpp"

namespace flightsim {

void PropertyBus::set(const std::string& key, double value) {
    m_data[key] = value;
}

double PropertyBus::get(const std::string& key, double fallback) const {
    auto it = m_data.find(key);
    if (it != m_data.end()) {
        return it->second;
    }
    return fallback;
}

bool PropertyBus::has(const std::string& key) const {
    return m_data.find(key) != m_data.end();
}

}
