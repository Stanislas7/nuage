#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "utils/config_loader.hpp"
#include "utils/json.hpp"
#include "math/vec3.hpp"

namespace {
constexpr double kDegToRad = 3.141592653589793 / 180.0;
constexpr double kRadToDeg = 180.0 / 3.141592653589793;
constexpr double kEarthRadiusM = 6378137.0;
constexpr double kFtToM = 0.3048;

struct Origin {
    double latDeg = 0.0;
    double lonDeg = 0.0;
    double altMeters = 0.0;
};

struct BoundsLLA {
    double minLat = 0.0;
    double minLon = 0.0;
    double maxLat = 0.0;
    double maxLon = 0.0;
    bool valid = false;
};

bool parseCsvLine(std::istream& in, std::vector<std::string>& out) {
    out.clear();
    std::string field;
    bool inQuotes = false;
    bool hadData = false;

    for (;;) {
        int ch = in.get();
        if (ch == EOF) {
            if (!field.empty() || hadData) {
                out.push_back(field);
                return true;
            }
            return false;
        }
        hadData = true;
        char c = static_cast<char>(ch);
        if (inQuotes) {
            if (c == '"') {
                if (in.peek() == '"') {
                    in.get();
                    field.push_back('"');
                } else {
                    inQuotes = false;
                }
            } else {
                field.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                out.push_back(field);
                field.clear();
            } else if (c == '\n') {
                if (!field.empty() && field.back() == '\r') {
                    field.pop_back();
                }
                out.push_back(field);
                return true;
            } else {
                field.push_back(c);
            }
        }
    }
}

bool parseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtod(s.c_str(), &end);
    return end != s.c_str();
}

nuage::Vec3 llaToEnu(const Origin& origin, double latDeg, double lonDeg, double altMeters) {
    double dLat = (latDeg - origin.latDeg) * kDegToRad;
    double dLon = (lonDeg - origin.lonDeg) * kDegToRad;
    double lat0Rad = origin.latDeg * kDegToRad;

    double east = dLon * kEarthRadiusM * std::cos(lat0Rad);
    double north = dLat * kEarthRadiusM;
    double up = altMeters - origin.altMeters;
    return nuage::Vec3(static_cast<float>(east),
                       static_cast<float>(up),
                       static_cast<float>(north));
}

BoundsLLA boundsFromManifest(const nlohmann::json& manifest) {
    BoundsLLA bounds;
    if (!manifest.contains("originLLA") || !manifest["originLLA"].is_array() ||
        manifest["originLLA"].size() != 3) {
        return bounds;
    }
    if (!manifest.contains("boundsENU") || !manifest["boundsENU"].is_array() ||
        manifest["boundsENU"].size() != 4) {
        return bounds;
    }

    Origin origin;
    origin.latDeg = manifest["originLLA"][0].get<double>();
    origin.lonDeg = manifest["originLLA"][1].get<double>();
    origin.altMeters = manifest["originLLA"][2].get<double>();

    double minX = manifest["boundsENU"][0].get<double>();
    double minZ = manifest["boundsENU"][1].get<double>();
    double maxX = manifest["boundsENU"][2].get<double>();
    double maxZ = manifest["boundsENU"][3].get<double>();

    double lat0Rad = origin.latDeg * kDegToRad;
    double metersPerLon = kEarthRadiusM * std::cos(lat0Rad);
    double metersPerLat = kEarthRadiusM;

    double minLat = origin.latDeg + (minZ / metersPerLat) * kRadToDeg;
    double maxLat = origin.latDeg + (maxZ / metersPerLat) * kRadToDeg;
    double minLon = origin.lonDeg + (minX / metersPerLon) * kRadToDeg;
    double maxLon = origin.lonDeg + (maxX / metersPerLon) * kRadToDeg;

    bounds.minLat = std::min(minLat, maxLat);
    bounds.maxLat = std::max(minLat, maxLat);
    bounds.minLon = std::min(minLon, maxLon);
    bounds.maxLon = std::max(minLon, maxLon);
    bounds.valid = true;
    return bounds;
}

bool withinBounds(double lat, double lon, const BoundsLLA& bounds) {
    if (!bounds.valid) return true;
    return lat >= bounds.minLat && lat <= bounds.maxLat &&
           lon >= bounds.minLon && lon <= bounds.maxLon;
}

void printUsage() {
    std::cout << "Usage: ourairports_import --airports <airports.csv> --runways <runways.csv>\n"
              << "                          --manifest <manifest.json> --out <output.json>\n"
              << "                          [--min-lat <deg> --min-lon <deg> --max-lat <deg> --max-lon <deg>]\n";
}
}

int main(int argc, char** argv) {
    std::string airportsPath;
    std::string runwaysPath;
    std::string manifestPath;
    std::string outPath;
    BoundsLLA boundsOverride;
    bool boundsOverrideSet = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](std::string& out) -> bool {
            if (i + 1 >= argc) return false;
            out = argv[++i];
            return true;
        };
        if (arg == "--airports") {
            if (!next(airportsPath)) return 1;
        } else if (arg == "--runways") {
            if (!next(runwaysPath)) return 1;
        } else if (arg == "--manifest") {
            if (!next(manifestPath)) return 1;
        } else if (arg == "--out") {
            if (!next(outPath)) return 1;
        } else if (arg == "--min-lat") {
            std::string v;
            if (!next(v)) return 1;
            boundsOverride.minLat = std::stod(v);
            boundsOverrideSet = true;
        } else if (arg == "--min-lon") {
            std::string v;
            if (!next(v)) return 1;
            boundsOverride.minLon = std::stod(v);
            boundsOverrideSet = true;
        } else if (arg == "--max-lat") {
            std::string v;
            if (!next(v)) return 1;
            boundsOverride.maxLat = std::stod(v);
            boundsOverrideSet = true;
        } else if (arg == "--max-lon") {
            std::string v;
            if (!next(v)) return 1;
            boundsOverride.maxLon = std::stod(v);
            boundsOverrideSet = true;
        } else {
            std::cerr << "Unknown arg: " << arg << "\n";
            printUsage();
            return 1;
        }
    }

    if (airportsPath.empty() || runwaysPath.empty() || manifestPath.empty() || outPath.empty()) {
        printUsage();
        return 1;
    }

    auto manifestOpt = nuage::loadJsonConfig(manifestPath);
    if (!manifestOpt) {
        std::cerr << "Failed to load manifest: " << manifestPath << "\n";
        return 1;
    }
    const auto& manifest = *manifestOpt;

    if (!manifest.contains("originLLA") || !manifest["originLLA"].is_array() ||
        manifest["originLLA"].size() != 3) {
        std::cerr << "Manifest missing originLLA.\n";
        return 1;
    }

    Origin origin;
    origin.latDeg = manifest["originLLA"][0].get<double>();
    origin.lonDeg = manifest["originLLA"][1].get<double>();
    origin.altMeters = manifest["originLLA"][2].get<double>();

    BoundsLLA bounds = boundsOverrideSet ? boundsOverride : boundsFromManifest(manifest);
    if (boundsOverrideSet) {
        bounds.valid = true;
        if (bounds.minLat > bounds.maxLat) std::swap(bounds.minLat, bounds.maxLat);
        if (bounds.minLon > bounds.maxLon) std::swap(bounds.minLon, bounds.maxLon);
    }

    std::ifstream airportsFile(airportsPath);
    if (!airportsFile.is_open()) {
        std::cerr << "Failed to open airports.csv: " << airportsPath << "\n";
        return 1;
    }
    std::ifstream runwaysFile(runwaysPath);
    if (!runwaysFile.is_open()) {
        std::cerr << "Failed to open runways.csv: " << runwaysPath << "\n";
        return 1;
    }

    std::unordered_map<std::string, int> airportHeader;
    std::unordered_map<std::string, int> runwayHeader;
    std::vector<std::string> fields;

    if (!parseCsvLine(airportsFile, fields)) {
        std::cerr << "airports.csv is empty.\n";
        return 1;
    }
    for (size_t i = 0; i < fields.size(); ++i) {
        airportHeader[fields[i]] = static_cast<int>(i);
    }

    if (!parseCsvLine(runwaysFile, fields)) {
        std::cerr << "runways.csv is empty.\n";
        return 1;
    }
    for (size_t i = 0; i < fields.size(); ++i) {
        runwayHeader[fields[i]] = static_cast<int>(i);
    }

    auto idx = [&](const std::unordered_map<std::string, int>& map, const char* key) -> int {
        auto it = map.find(key);
        if (it == map.end()) return -1;
        return it->second;
    };

    int aIdent = idx(airportHeader, "ident");
    int aName = idx(airportHeader, "name");
    int aType = idx(airportHeader, "type");
    int aLat = idx(airportHeader, "latitude_deg");
    int aLon = idx(airportHeader, "longitude_deg");
    int aElev = idx(airportHeader, "elevation_ft");

    int rAirport = idx(runwayHeader, "airport_ident");
    int rLeIdent = idx(runwayHeader, "le_ident");
    int rHeIdent = idx(runwayHeader, "he_ident");
    int rLeLat = idx(runwayHeader, "le_latitude_deg");
    int rLeLon = idx(runwayHeader, "le_longitude_deg");
    int rLeElev = idx(runwayHeader, "le_elevation_ft");
    int rHeLat = idx(runwayHeader, "he_latitude_deg");
    int rHeLon = idx(runwayHeader, "he_longitude_deg");
    int rHeElev = idx(runwayHeader, "he_elevation_ft");
    int rLength = idx(runwayHeader, "length_ft");
    int rWidth = idx(runwayHeader, "width_ft");
    int rSurface = idx(runwayHeader, "surface");
    int rLighted = idx(runwayHeader, "lighted");
    int rClosed = idx(runwayHeader, "closed");

    nlohmann::json out;
    out["originLLA"] = {origin.latDeg, origin.lonDeg, origin.altMeters};
    if (bounds.valid) {
        out["boundsLLA"] = {bounds.minLat, bounds.minLon, bounds.maxLat, bounds.maxLon};
    }
    out["airports"] = nlohmann::json::array();
    out["runways"] = nlohmann::json::array();

    std::unordered_set<std::string> selectedAirports;

    while (parseCsvLine(airportsFile, fields)) {
        if (fields.empty()) continue;
        auto get = [&](int i) -> std::string {
            if (i < 0 || static_cast<size_t>(i) >= fields.size()) return {};
            return fields[static_cast<size_t>(i)];
        };

        std::string ident = get(aIdent);
        if (ident.empty()) continue;

        std::string type = get(aType);
        if (type == "heliport") {
            continue;
        }

        double lat = 0.0;
        double lon = 0.0;
        if (!parseDouble(get(aLat), lat) || !parseDouble(get(aLon), lon)) {
            continue;
        }
        if (!withinBounds(lat, lon, bounds)) {
            continue;
        }

        double elevFt = 0.0;
        parseDouble(get(aElev), elevFt);
        double elevM = elevFt * kFtToM;
        nuage::Vec3 enu = llaToEnu(origin, lat, lon, elevM);

        nlohmann::json entry;
        entry["ident"] = ident;
        entry["name"] = get(aName);
        entry["type"] = type;
        entry["latitudeDeg"] = lat;
        entry["longitudeDeg"] = lon;
        entry["elevationFt"] = elevFt;
        entry["positionENU"] = {enu.x, enu.y, enu.z};
        out["airports"].push_back(entry);
        selectedAirports.insert(ident);
    }

    while (parseCsvLine(runwaysFile, fields)) {
        if (fields.empty()) continue;
        auto get = [&](int i) -> std::string {
            if (i < 0 || static_cast<size_t>(i) >= fields.size()) return {};
            return fields[static_cast<size_t>(i)];
        };

        std::string airportIdent = get(rAirport);
        if (airportIdent.empty()) continue;
        if (!selectedAirports.empty() && selectedAirports.find(airportIdent) == selectedAirports.end()) {
            continue;
        }

        double leLat = 0.0, leLon = 0.0, leElevFt = 0.0;
        double heLat = 0.0, heLon = 0.0, heElevFt = 0.0;
        if (!parseDouble(get(rLeLat), leLat) || !parseDouble(get(rLeLon), leLon)) {
            continue;
        }
        if (!parseDouble(get(rHeLat), heLat) || !parseDouble(get(rHeLon), heLon)) {
            continue;
        }
        parseDouble(get(rLeElev), leElevFt);
        parseDouble(get(rHeElev), heElevFt);

        double leElevM = leElevFt * kFtToM;
        double heElevM = heElevFt * kFtToM;
        nuage::Vec3 leEnu = llaToEnu(origin, leLat, leLon, leElevM);
        nuage::Vec3 heEnu = llaToEnu(origin, heLat, heLon, heElevM);

        nlohmann::json entry;
        entry["airportIdent"] = airportIdent;
        entry["leIdent"] = get(rLeIdent);
        entry["heIdent"] = get(rHeIdent);
        entry["leLatitudeDeg"] = leLat;
        entry["leLongitudeDeg"] = leLon;
        entry["leElevationFt"] = leElevFt;
        entry["heLatitudeDeg"] = heLat;
        entry["heLongitudeDeg"] = heLon;
        entry["heElevationFt"] = heElevFt;
        entry["leENU"] = {leEnu.x, leEnu.y, leEnu.z};
        entry["heENU"] = {heEnu.x, heEnu.y, heEnu.z};
        entry["lengthFt"] = get(rLength);
        entry["widthFt"] = get(rWidth);
        entry["surface"] = get(rSurface);
        entry["lighted"] = get(rLighted);
        entry["closed"] = get(rClosed);
        out["runways"].push_back(entry);
    }

    std::ofstream outFile(outPath);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open output: " << outPath << "\n";
        return 1;
    }
    outFile << out.dump(2) << "\n";

    std::cout << "Wrote " << out["airports"].size() << " airports and "
              << out["runways"].size() << " runways to " << outPath << "\n";
    return 0;
}
