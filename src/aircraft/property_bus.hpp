#pragma once

#include <string>
#include <unordered_map>

namespace flightsim {

class PropertyBus {
public:
    void set(const std::string& key, double value);
    double get(const std::string& key, double fallback = 0.0) const;
    bool has(const std::string& key) const;

private:
    std::unordered_map<std::string, double> m_data;
};

}
