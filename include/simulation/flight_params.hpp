#pragma once

#include <string>

namespace flightsim {

// Aircraft configuration - loaded from JSON
struct FlightParams {
    std::string name = "Default";

    // Speed limits
    float minSpeed = 20.0f;   // m/s (stall)
    float maxSpeed = 80.0f;   // m/s

    // Control rates (rad/s at full deflection)
    float pitchRate = 1.5f;
    float yawRate = 1.0f;
    float rollRate = 2.0f;

    // Limits
    float maxPitch = 1.2f;    // radians
    float minAltitude = 5.0f; // meters (ground)

    // Engine
    float throttleResponse = 0.5f;  // How fast throttle changes

    // Load from file
    static FlightParams load(const std::string& path);
};

}
