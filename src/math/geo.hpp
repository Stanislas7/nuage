#pragma once

#include "math/vec3.hpp"

namespace nuage {

struct GeoOrigin {
    double latDeg = 0.0;
    double lonDeg = 0.0;
    double altMeters = 0.0;
};

Vec3 llaToEnu(const GeoOrigin& origin, double latDeg, double lonDeg, double altMeters);
void enuToLla(const GeoOrigin& origin, const Vec3& enu, double& outLatDeg, double& outLonDeg, double& outAltMeters);

} // namespace nuage
