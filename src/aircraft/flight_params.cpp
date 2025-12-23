#include "aircraft/flight_params.hpp"
#include <iostream>

namespace flightsim {

FlightParams FlightParams::load(const std::string& path) {
    // TODO: Load from JSON
    std::cout << "Loading flight params from " << path << " (Using defaults for now)\n";
    return FlightParams{};
}

}
