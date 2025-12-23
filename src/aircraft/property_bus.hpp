#pragma once

#include "math/vec3.hpp"
#include "math/quat.hpp"
#include <string>
#include <unordered_map>

namespace nuage {

class PropertyBus {
public:
    void set(const std::string& key, double value);
    double get(const std::string& key, double fallback = 0.0) const;
    bool has(const std::string& key) const;

    void setVec3(const std::string& prefix, double x, double y, double z);
    void setVec3(const std::string& prefix, const Vec3& v);
    Vec3 getVec3(const std::string& prefix) const;

    void setQuat(const std::string& prefix, const Quat& q);
    Quat getQuat(const std::string& prefix) const;

    void increment(const std::string& key, double delta);
    
private:
    std::unordered_map<std::string, double> m_data;
};

}
