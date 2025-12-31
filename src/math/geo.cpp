#include "math/geo.hpp"
#include <cmath>

namespace nuage {

namespace {
constexpr double kDegToRad = 3.141592653589793 / 180.0;
constexpr double kEarthRadiusM = 6378137.0;
}

Vec3 llaToEnu(const GeoOrigin& origin, double latDeg, double lonDeg, double altMeters) {
    double dLat = (latDeg - origin.latDeg) * kDegToRad;
    double dLon = (lonDeg - origin.lonDeg) * kDegToRad;
    double lat0Rad = origin.latDeg * kDegToRad;

    double east = dLon * kEarthRadiusM * std::cos(lat0Rad);
    double north = dLat * kEarthRadiusM;
    double up = altMeters - origin.altMeters;

    return Vec3(static_cast<float>(east),
                static_cast<float>(up),
                static_cast<float>(north));
}

void enuToLla(const GeoOrigin& origin, const Vec3& enu, double& outLatDeg, double& outLonDeg, double& outAltMeters) {
    double lat0Rad = origin.latDeg * kDegToRad;
    double metersPerLon = kEarthRadiusM * std::cos(lat0Rad);
    double metersPerLat = kEarthRadiusM;

    outLatDeg = origin.latDeg + (static_cast<double>(enu.z) / metersPerLat) * (180.0 / 3.141592653589793);
    outLonDeg = origin.lonDeg + (static_cast<double>(enu.x) / metersPerLon) * (180.0 / 3.141592653589793);
    outAltMeters = origin.altMeters + static_cast<double>(enu.y);
}

} // namespace nuage
