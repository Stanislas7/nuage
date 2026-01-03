#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <optional>

namespace nuage {

struct XmlNode {
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
    std::string text;
    std::vector<XmlNode> children;
};

std::optional<XmlNode> parseXml(const std::string& content);
std::optional<XmlNode> loadXmlFile(const std::string& path);

} // namespace nuage
