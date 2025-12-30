#pragma once

#include "aircraft/aircraft_component.hpp"
#include "math/vec3.hpp"
#include "math/quat.hpp"
#include <FGFDMExec.h>
#include <memory>
#include <string>

namespace nuage {

struct JsbsimConfig {
    std::string modelName = "c172p";
    std::string rootPath = "assets/jsbsim";
    double initLatDeg = 0.0;
    double initLonDeg = 0.0;
    double originLatDeg = 0.0;
    double originLonDeg = 0.0;
    bool hasOrigin = false;
};

class JsbsimSystem : public AircraftComponent {
public:
    explicit JsbsimSystem(JsbsimConfig config);

    const char* name() const override { return "JSBSimSystem"; }
    void init(AircraftState& state, PropertyContext& properties) override;
    void update(float dt) override;

private:
    AircraftState* m_acState = nullptr;
    PropertyContext* m_properties = nullptr;
    JsbsimConfig m_config;
    std::unique_ptr<JSBSim::FGFDMExec> m_fdm;
    bool m_initialized = false;
    double m_originLatRad = 0.0;
    double m_originLonRad = 0.0;

    void ensureInitialized(float dt);
    void syncInputs();
    void syncOutputs();
};

} // namespace nuage
