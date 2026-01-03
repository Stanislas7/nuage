#pragma once

#include "utils/xml.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace nuage {

struct FGMaterial {
    std::string canonicalName;
    std::vector<std::string> names;
    std::vector<std::string> textures;
    std::string effect;
    float xsize = 0.0f;
    float ysize = 0.0f;
};

struct FGLandclassEntry {
    int id = 0;
    std::string materialName;
    bool water = false;
    bool sea = false;
};

class FGMaterialLibrary {
public:
    bool loadFromRoot(const std::string& fgRoot);

    const std::unordered_map<std::string, FGMaterial>& materialsByName() const {
        return m_materialsByName;
    }
    const std::vector<FGLandclassEntry>& landclassEntries() const { return m_landclassEntries; }
    const std::array<std::uint8_t, 256>& landclassFlags() const { return m_landclassFlags; }

private:
    void collectFromFile(const std::filesystem::path& path);
    void collectFromNode(const XmlNode& node, const std::filesystem::path& baseDir);
    void parseMaterialNode(const XmlNode& node, const std::filesystem::path& baseDir);
    void parseLandclassNode(const XmlNode& node, const std::filesystem::path& baseDir);

    static bool isUrbanName(const std::string& name);
    static bool isForestName(const std::string& name);
    static std::string toLower(const std::string& in);

    std::filesystem::path m_root;
    std::unordered_map<std::string, FGMaterial> m_materialsByName;
    std::vector<FGLandclassEntry> m_landclassEntries;
    std::array<std::uint8_t, 256> m_landclassFlags{};
};

} // namespace nuage
