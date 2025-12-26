#pragma once

#include <string>

namespace nuage {

/**
 * @brief Parameters for starting a flight session.
 */
struct FlightConfig {
    std::string aircraftPath;
    std::string terrainPath;
    std::string sceneryPath;
    float timeOfDay = 12.0f; // 0.0 to 24.0
    
    // Optional: Weather, start position overrides, etc.
};

} // namespace nuage
