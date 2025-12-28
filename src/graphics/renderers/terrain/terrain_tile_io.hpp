#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace nuage {

bool load_compiled_mesh(const std::string& path, std::vector<float>& out);
bool load_compiled_mask(const std::string& path, int expectedRes, std::vector<std::uint8_t>& out);

} // namespace nuage
