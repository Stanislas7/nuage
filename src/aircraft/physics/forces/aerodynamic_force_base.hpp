#pragma once

#include "core/property_bus.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"

namespace nuage {

class AerodynamicForceBase {
public:
    virtual ~AerodynamicForceBase() = default;

protected:
    struct AerodynamicData {
        Vec3 velocity;
        float speed;
        float dynamicPressure;
        Vec3 airflowDir;
        float airDensity;
        Quat orientation;
        Vec3 forward;
        Vec3 up;
        Vec3 right;
    };

    AerodynamicData computeAerodynamics(PropertyBus* state) const;
};

}
