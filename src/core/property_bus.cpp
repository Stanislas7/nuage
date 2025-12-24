#include "core/property_bus.hpp"

namespace nuage {

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

void PropertyBus::setVec3(const std::string& prefix, double x, double y, double z) {
    set(prefix + "/x", x);
    set(prefix + "/y", y);
    set(prefix + "/z", z);
}

void PropertyBus::setVec3(const std::string& prefix, const Vec3& v) {
    setVec3(prefix, v.x, v.y, v.z);
}

Vec3 PropertyBus::getVec3(const std::string& prefix) const {
    return Vec3(
        get(prefix + "/x", 0.0),
        get(prefix + "/y", 0.0),
        get(prefix + "/z", 0.0)
    );
}

void PropertyBus::setQuat(const std::string& prefix, const Quat& q) {
    set(prefix + "/w", q.w);
    set(prefix + "/x", q.x);
    set(prefix + "/y", q.y);
    set(prefix + "/z", q.z);
}

Quat PropertyBus::getQuat(const std::string& prefix) const {
    return Quat(
        get(prefix + "/w", 1.0),
        get(prefix + "/x", 0.0),
        get(prefix + "/y", 0.0),
        get(prefix + "/z", 0.0)
    );
}

void PropertyBus::increment(const std::string& key, double delta) {
    m_data[key] = get(key) + delta;
}

}
