#pragma once

#include "math/vec3.hpp"

namespace nuage {

class Atmosphere {
public:
    void init();
    void update(float dt);

    float getAirDensity(float altitude) const;
    Vec3 getWind(const Vec3& position) const;

    float getTimeOfDay() const { return m_timeOfDay; }
    Vec3 getSunDirection() const;

    void setTimeOfDay(float hours);
    void setWind(float speed, float heading);

private:
    float m_timeOfDay = 12.0f;
    float m_windSpeed = 0.0f;
    float m_windHeading = 0.0f;
};

}
