#include "graphics/renderers/terrain/fg_materials.hpp"
#include <cctype>
#include <iostream>

namespace nuage {
namespace {

bool parseFloat(const std::string& text, float& out) {
    try {
        out = std::stof(text);
        return true;
    } catch (...) {
        return false;
    }
}

void collectMaterialFields(const XmlNode& node, FGMaterial& material) {
    if (node.name == "name") {
        if (!node.text.empty()) {
            material.names.push_back(node.text);
        }
    } else if (node.name == "texture") {
        if (!node.text.empty()) {
            material.textures.push_back(node.text);
        }
    } else if (node.name == "effect") {
        if (!node.text.empty()) {
            material.effect = node.text;
        }
    } else if (node.name == "xsize") {
        float value = 0.0f;
        if (parseFloat(node.text, value)) {
            material.xsize = value;
        }
    } else if (node.name == "ysize") {
        float value = 0.0f;
        if (parseFloat(node.text, value)) {
            material.ysize = value;
        }
    }
    for (const auto& child : node.children) {
        collectMaterialFields(child, material);
    }
}

void collectLandclassFields(const XmlNode& node, FGLandclassEntry& entry) {
    if (node.name == "landclass") {
        if (!node.text.empty()) {
            try {
                entry.id = std::stoi(node.text);
            } catch (...) {
                entry.id = 0;
            }
        }
    } else if (node.name == "material-name") {
        entry.materialName = node.text;
    } else if (node.name == "water") {
        entry.water = (node.text == "true" || node.text == "1");
    } else if (node.name == "sea") {
        entry.sea = (node.text == "true" || node.text == "1");
    }
    for (const auto& child : node.children) {
        collectLandclassFields(child, entry);
    }
}

} // namespace

bool FGMaterialLibrary::loadFromRoot(const std::string& fgRoot) {
    m_root = std::filesystem::path(fgRoot);
    m_materialsByName.clear();
    m_landclassEntries.clear();
    m_landclassFlags.fill(0);

    std::filesystem::path materialsPath = m_root / "Materials" / "default" / "materials.xml";
    collectFromFile(materialsPath);

    for (const auto& entry : m_landclassEntries) {
        if (entry.id < 0 || entry.id >= static_cast<int>(m_landclassFlags.size())) {
            continue;
        }
        std::uint8_t flags = 0;
        if (entry.water || entry.sea) {
            flags |= 0x1;
        }
        std::string name = toLower(entry.materialName);
        if (isUrbanName(name)) {
            flags |= 0x2;
        }
        if (isForestName(name)) {
            flags |= 0x4;
        }
        m_landclassFlags[static_cast<size_t>(entry.id)] = flags;
    }

    return !m_materialsByName.empty();
}

void FGMaterialLibrary::collectFromFile(const std::filesystem::path& path) {
    auto rootOpt = loadXmlFile(path.string());
    if (!rootOpt) {
        std::cerr << "[fg] failed to parse XML: " << path << "\n";
        return;
    }
    collectFromNode(*rootOpt, path.parent_path());
}

void FGMaterialLibrary::collectFromNode(const XmlNode& node, const std::filesystem::path& baseDir) {
    auto includeIt = node.attributes.find("include");
    if (includeIt != node.attributes.end()) {
        std::filesystem::path includePath = includeIt->second;
        if (includePath.is_relative()) {
            includePath = m_root / includePath;
        }
        auto includeRoot = loadXmlFile(includePath.string());
        if (includeRoot) {
            collectFromNode(*includeRoot, includePath.parent_path());
        } else {
            std::cerr << "[fg] failed to load include: " << includePath << "\n";
        }
        if (node.name == "material") {
            parseMaterialNode(node, baseDir);
        } else if (node.name == "landclass-mapping") {
            parseLandclassNode(node, baseDir);
        }
        return;
    }

    if (node.name == "material") {
        parseMaterialNode(node, baseDir);
    } else if (node.name == "landclass-mapping") {
        parseLandclassNode(node, baseDir);
    }

    for (const auto& child : node.children) {
        collectFromNode(child, baseDir);
    }
}

void FGMaterialLibrary::parseMaterialNode(const XmlNode& node, const std::filesystem::path& baseDir) {
    (void)baseDir;
    FGMaterial material;
    auto includeIt = node.attributes.find("include");
    if (includeIt != node.attributes.end()) {
        std::filesystem::path includePath = includeIt->second;
        if (includePath.is_relative()) {
            includePath = m_root / includePath;
        }
        auto includeRoot = loadXmlFile(includePath.string());
        if (includeRoot) {
            collectMaterialFields(*includeRoot, material);
        }
    }

    collectMaterialFields(node, material);
    if (material.names.empty()) {
        return;
    }
    material.canonicalName = material.names.front();
    for (const auto& name : material.names) {
        if (name.empty()) {
            continue;
        }
        m_materialsByName[name] = material;
    }
}

void FGMaterialLibrary::parseLandclassNode(const XmlNode& node, const std::filesystem::path& baseDir) {
    (void)baseDir;
    auto includeIt = node.attributes.find("include");
    if (includeIt != node.attributes.end()) {
        std::filesystem::path includePath = includeIt->second;
        if (includePath.is_relative()) {
            includePath = m_root / includePath;
        }
        auto includeRoot = loadXmlFile(includePath.string());
        if (includeRoot) {
            parseLandclassNode(*includeRoot, includePath.parent_path());
        }
    }

    if (node.name == "map") {
        FGLandclassEntry entry;
        collectLandclassFields(node, entry);
        if (!entry.materialName.empty()) {
            m_landclassEntries.push_back(entry);
        }
    }

    for (const auto& child : node.children) {
        parseLandclassNode(child, baseDir);
    }
}

std::string FGMaterialLibrary::toLower(const std::string& in) {
    std::string out = in;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

bool FGMaterialLibrary::isUrbanName(const std::string& name) {
    static const std::vector<std::string> tokens = {
        "urban", "suburban", "industrial", "transport", "port",
        "airport", "construction", "town", "city", "built", "settlement"
    };
    for (const auto& token : tokens) {
        if (name.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool FGMaterialLibrary::isForestName(const std::string& name) {
    static const std::vector<std::string> tokens = {
        "forest", "wood", "deciduous", "evergreen", "mixedforest", "rainforest"
    };
    for (const auto& token : tokens) {
        if (name.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace nuage
