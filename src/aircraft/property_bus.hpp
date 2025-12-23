#pragma once

#include <string>
#include <unordered_map>

namespace nuage {

class PropertyBus {
public:
    void set(const std::string& key, double value);
    double get(const std::string& key, double fallback = 0.0) const;
    bool has(const std::string& key) const;

    void setVec3(const std::string& prefix, double x, double y, double z);
    
private:
    std::unordered_map<std::string, double> m_data;
};

}
