#pragma once

#include "utils/json.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <optional>

namespace nuage {
    using json = nlohmann::json;

    inline std::optional<json> loadJsonConfig(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << path << std::endl;
            return std::nullopt;
        }
        try {
            json j;
            file >> j;
            return j;
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse JSON from " << path << ": " << e.what() << std::endl;
            return std::nullopt;
        }
    }
}
