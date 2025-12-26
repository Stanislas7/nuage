#include "environment/atmosphere.hpp"
#include <cmath>

namespace nuage {

void Atmosphere::init() {
    m_timeOfDay = 12.0f;
    m_windSpeed = 0.0f;
    m_windHeading = 0.0f;
}

void Atmosphere::update(float dt) {
    // Could advance time of day here if desired
}

float Atmosphere::getAirDensity(float altitude) const {
    constexpr float seaLevelDensity = 1.225f;
    constexpr float scaleHeight = 8500.0f;
    
    return seaLevelDensity * std::exp(-altitude / scaleHeight);
}

Vec3 Atmosphere::getWind(const Vec3& position) const {
    if (m_windSpeed <= 0.0f) {
        return Vec3(0, 0, 0);
    }
    
    float headingRad = m_windHeading * 3.14159265f / 180.0f;
    return Vec3(
        std::sin(headingRad) * m_windSpeed,
        0.0f,
        std::cos(headingRad) * m_windSpeed
    );
}

Vec3 Atmosphere::getSunDirection() const {
    float hourAngle = (m_timeOfDay - 12.0f) * 15.0f * 3.14159265f / 180.0f;
    
    return Vec3(
        std::sin(hourAngle),
        std::cos(hourAngle * 0.5f),
        0.0f
    ).normalized();
}

void Atmosphere::setTimeOfDay(float hours) {
    m_timeOfDay = std::fmod(hours, 24.0f);
    if (m_timeOfDay < 0) m_timeOfDay += 24.0f;
}

void Atmosphere::setWind(float speed, float heading) {
    m_windSpeed = speed;
    m_windHeading = heading;
}

}
