#include "jsbsim_system.hpp"
#include "core/properties/property_context.hpp"
#include "core/properties/property_paths.hpp"
#include "aircraft/aircraft_state.hpp"
#include "math/geo.hpp"
#include "graphics/renderers/terrain_renderer.hpp"
#include <FGFDMExec.h>
#include <input_output/FGGroundCallback.h>
#include <models/FGInertial.h>
#include <models/FGPropagate.h>
#include <math/FGColumnVector3.h>
#include <math/FGMatrix33.h>
#include <math/FGLocation.h>
#include <simgear/misc/sg_path.hxx>
#include <initialization/FGInitialCondition.h>
#include <algorithm>
#include <cmath>

namespace nuage {

namespace {
constexpr double kFtToM = 0.3048;
constexpr double kMToFt = 1.0 / kFtToM;
constexpr double kEarthRadiusM = 6378137.0; // WGS84 semi-major
constexpr double kDegToRad = 3.141592653589793 / 180.0;
constexpr double kWgs84SemiMajorFt = 6378137.0 / kFtToM;
constexpr double kWgs84SemiMinorFt = 6356752.3142 / kFtToM;

float clampInput(double v) {
    return static_cast<float>(std::clamp(v, -1.0, 1.0));
}

Vec3 nedToWorld(const JSBSim::FGColumnVector3& ned) {
    float north = static_cast<float>(ned(1)) * static_cast<float>(kFtToM);
    float east = static_cast<float>(ned(2)) * static_cast<float>(kFtToM);
    float down = static_cast<float>(ned(3)) * static_cast<float>(kFtToM);
    return Vec3(east, -down, north);
}

Vec3 jsbBodyToNuage(const JSBSim::FGColumnVector3& body) {
    // JSBSim body axes: X forward, Y right, Z down
    // Nuage body axes:  X right,   Y up,    Z forward
    return Vec3(
        static_cast<float>(body(2)),       // right
        static_cast<float>(-body(3)),      // up
        static_cast<float>(body(1))        // forward
    );
}

Quat quatFromMatrix(const float m[3][3]) {
    float trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        float s = std::sqrt(trace + 1.0f) * 2.0f;
        float w = 0.25f * s;
        float x = (m[2][1] - m[1][2]) / s;
        float y = (m[0][2] - m[2][0]) / s;
        float z = (m[1][0] - m[0][1]) / s;
        return Quat(w, x, y, z).normalized();
    }
    if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
        float s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        float w = (m[2][1] - m[1][2]) / s;
        float x = 0.25f * s;
        float y = (m[0][1] + m[1][0]) / s;
        float z = (m[0][2] + m[2][0]) / s;
        return Quat(w, x, y, z).normalized();
    }
    if (m[1][1] > m[2][2]) {
        float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        float w = (m[0][2] - m[2][0]) / s;
        float x = (m[0][1] + m[1][0]) / s;
        float y = 0.25f * s;
        float z = (m[1][2] + m[2][1]) / s;
        return Quat(w, x, y, z).normalized();
    }
    float s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
    float w = (m[1][0] - m[0][1]) / s;
    float x = (m[0][2] + m[2][0]) / s;
    float y = (m[1][2] + m[2][1]) / s;
    float z = 0.25f * s;
    return Quat(w, x, y, z).normalized();
}

class TerrainGroundCallback : public JSBSim::FGGroundCallback {
public:
    TerrainGroundCallback(const TerrainRenderer* terrain, GeoOrigin origin)
        : m_terrain(terrain), m_origin(origin) {}

    double GetAGLevel(double t, const JSBSim::FGLocation& location,
                      JSBSim::FGLocation& contact,
                      JSBSim::FGColumnVector3& normal,
                      JSBSim::FGColumnVector3& v,
                      JSBSim::FGColumnVector3& w) const override {
        (void)t;
        v.InitMatrix();
        w.InitMatrix();

        JSBSim::FGLocation l = location;
        l.SetEllipse(kWgs84SemiMajorFt, kWgs84SemiMinorFt);
        double latRad = l.GetGeodLatitudeRad();
        double lonRad = l.GetLongitude();
        double sinLat = std::sin(latRad);
        double cosLat = std::cos(latRad);
        double sinLon = std::sin(lonRad);
        double cosLon = std::cos(lonRad);

        double latDeg = l.GetGeodLatitudeDeg();
        double lonDeg = l.GetLongitudeDeg();
        Vec3 enu = llaToEnu(m_origin, latDeg, lonDeg, m_origin.altMeters);

        TerrainRenderer::TerrainSample sample;
        bool hasSample = m_terrain && m_terrain->sampleSurfaceNoLoad(enu.x, enu.z, sample);
        if (!hasSample && m_terrain) {
            // Allow a one-off load if the cached ring missed; avoids zero height on tall terrain.
            hasSample = m_terrain->sampleSurface(enu.x, enu.z, sample);
        }
        if (!hasSample) {
            if (m_hasLastHeight) {
                sample.height = m_lastHeightMeters;
                sample.normal = m_lastNormal;
            } else {
                sample.height = 0.0f;
                sample.normal = Vec3(0.0f, 1.0f, 0.0f);
            }
        } else {
            m_lastHeightMeters = sample.height;
            m_lastNormal = sample.normal;
            m_hasLastHeight = true;
        }

        auto enuToEcef = [&](const Vec3& n) {
            double e = static_cast<double>(n.x);
            double u = static_cast<double>(n.y);
            double nn = static_cast<double>(n.z);
            double x = -sinLon * e - sinLat * cosLon * nn + cosLat * cosLon * u;
            double y =  cosLon * e - sinLat * sinLon * nn + cosLat * sinLon * u;
            double z =  cosLat * nn + sinLat * u;
            return JSBSim::FGColumnVector3(x, y, z);
        };
        normal = enuToEcef(sample.normal);

        double groundFt = static_cast<double>(sample.height) * kMToFt;
        contact.SetEllipse(kWgs84SemiMajorFt, kWgs84SemiMinorFt);
        contact.SetPositionGeodetic(lonRad, latRad, groundFt);

        return l.GetGeodAltitude() - groundFt;
    }

private:
    const TerrainRenderer* m_terrain = nullptr;
    GeoOrigin m_origin;
    mutable float m_lastHeightMeters = 0.0f;
    mutable bool m_hasLastHeight = false;
    mutable Vec3 m_lastNormal = Vec3(0.0f, 1.0f, 0.0f);
};
}

JsbsimSystem::JsbsimSystem(JsbsimConfig config)
    : m_config(std::move(config))
{
}

void JsbsimSystem::init(AircraftState& state, PropertyContext& properties) {
    m_acState = &state;
    m_properties = &properties;
    double originLat = m_config.hasOrigin ? m_config.originLatDeg : m_config.initLatDeg;
    double originLon = m_config.hasOrigin ? m_config.originLonDeg : m_config.initLonDeg;
    m_originLatRad = originLat * 3.1415926535 / 180.0;
    m_originLonRad = originLon * 3.1415926535 / 180.0;
}

void JsbsimSystem::ensureInitialized(float dt) {
    if (m_initialized) return;

    m_fdm = std::make_unique<JSBSim::FGFDMExec>();
    SGPath rootPath(m_config.rootPath);
    m_fdm->SetRootDir(rootPath);
    m_fdm->SetAircraftPath(SGPath("aircraft"));
    m_fdm->SetEnginePath(SGPath("engine"));
    m_fdm->SetSystemsPath(SGPath("systems"));
    m_fdm->Setdt(dt);

    // Initial conditions derived from current state
    m_fdm->SetPropertyValue("ic/long-gc-deg", m_config.initLonDeg);
    m_fdm->SetPropertyValue("ic/lat-gc-deg", m_config.initLatDeg);
    m_fdm->SetPropertyValue("ic/h-sl-ft", m_acState->position.y * kMToFt);
    m_fdm->SetPropertyValue("ic/psi-true-deg", 0.0);
    m_fdm->SetPropertyValue("ic/theta-deg", 0.0);
    m_fdm->SetPropertyValue("ic/phi-deg", 0.0);

    // Map Nuage velocity (east, up, north) to JSBSim body u/v/w
    m_fdm->SetPropertyValue("ic/u-fps", m_acState->velocity.z * kMToFt);   // forward along body X
    m_fdm->SetPropertyValue("ic/v-fps", m_acState->velocity.x * kMToFt);   // right
    m_fdm->SetPropertyValue("ic/w-fps", -m_acState->velocity.y * kMToFt);  // down

    if (!m_fdm->LoadModel(m_config.modelName)) {
        return;
    }

    auto inertial = m_fdm->GetInertial();
    if (inertial && m_config.terrain) {
        GeoOrigin origin;
        origin.latDeg = m_config.hasOrigin ? m_config.originLatDeg : m_config.initLatDeg;
        origin.lonDeg = m_config.hasOrigin ? m_config.originLonDeg : m_config.initLonDeg;
        origin.altMeters = m_config.originAltMeters;
        inertial->SetGroundCallback(new TerrainGroundCallback(m_config.terrain, origin));
        m_hasGroundCallback = true;
    }

    m_fdm->RunIC();
    m_initialized = true;
}

void JsbsimSystem::syncInputs() {
    PropertyBus& local = m_properties->local();
    double elevator = local.get(Properties::Controls::ELEVATOR);
    double aileron = local.get(Properties::Controls::AILERON);
    double rudder = local.get(Properties::Controls::RUDDER);
    double throttle = local.get(Properties::Controls::THROTTLE);

    m_fdm->SetPropertyValue("fcs/elevator-cmd-norm", clampInput(elevator));
    m_fdm->SetPropertyValue("fcs/aileron-cmd-norm", clampInput(-aileron));
    m_fdm->SetPropertyValue("fcs/rudder-cmd-norm", clampInput(rudder));
    m_fdm->SetPropertyValue("fcs/throttle-cmd-norm", clampInput(throttle));

    Vec3 wind = local.get(Properties::Atmosphere::WIND_PREFIX);
    double windNorth = wind.z * kMToFt;
    double windEast = wind.x * kMToFt;
    double windDown = -wind.y * kMToFt;
    m_fdm->SetPropertyValue("atmosphere/wind-north-fps", windNorth);
    m_fdm->SetPropertyValue("atmosphere/wind-east-fps", windEast);
    m_fdm->SetPropertyValue("atmosphere/wind-down-fps", windDown);
}

void JsbsimSystem::syncOutputs() {
    std::shared_ptr<JSBSim::FGPropagate> prop = m_fdm->GetPropagate();
    if (!prop) {
        return;
    }

    const auto& velNed = prop->GetVel();
    m_acState->velocity = nedToWorld(velNed);

    const auto& pqr = prop->GetPQR();
    m_acState->angularVelocity = jsbBodyToNuage(pqr);

    // Orientation: JSBSim body -> NED matrix, then align body axes to Nuage
    JSBSim::FGMatrix33 b2l = prop->GetTb2l();
    float b2lMat[3][3];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            b2lMat[r][c] = static_cast<float>(b2l(r + 1, c + 1));
        }
    }
    const float jsbToNuageBodyT[3][3] = {
        {0.f, 0.f, 1.f},
        {1.f, 0.f, 0.f},
        {0.f, -1.f, 0.f}
    };
    float b2n[3][3] = {};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            b2n[r][c] = b2lMat[r][0] * jsbToNuageBodyT[0][c]
                      + b2lMat[r][1] * jsbToNuageBodyT[1][c]
                      + b2lMat[r][2] * jsbToNuageBodyT[2][c];
        }
    }
    const float nedToWorldMat[3][3] = {
        {0.f, 1.f, 0.f},
        {0.f, 0.f, -1.f},
        {1.f, 0.f, 0.f}
    };
    float b2w[3][3] = {};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            b2w[r][c] = nedToWorldMat[r][0] * b2n[0][c]
                      + nedToWorldMat[r][1] * b2n[1][c]
                      + nedToWorldMat[r][2] * b2n[2][c];
        }
    }
    m_acState->orientation = quatFromMatrix(b2w);

    double lat = m_fdm->GetPropertyValue("position/lat-gc-rad");
    double lon = m_fdm->GetPropertyValue("position/long-gc-rad");
    double altFt = m_fdm->GetPropertyValue("position/h-sl-ft");

    double dLat = lat - m_originLatRad;
    double dLon = lon - m_originLonRad;

    double north = dLat * kEarthRadiusM;
    double east = dLon * kEarthRadiusM * std::cos(m_originLatRad);
    double alt = altFt * kFtToM;

    m_acState->position = Vec3(
        static_cast<float>(east),
        static_cast<float>(alt),
        static_cast<float>(north)
    );

    double airspeedFps = m_fdm->GetPropertyValue("velocities/vtrue-fps");
    m_acState->airspeed = airspeedFps * kFtToM;

    // Publish to Property Bus
    PropertyBus& local = m_properties->local();
    local.set(Properties::Velocities::AIRSPEED_KT, airspeedFps * 0.592484); // fps to knots
    local.set(Properties::Position::ALTITUDE_FT, altFt);
    local.set(Properties::Position::LATITUDE_DEG, lat * 180.0 / 3.1415926535);
    local.set(Properties::Position::LONGITUDE_DEG, lon * 180.0 / 3.1415926535);
    
    local.set(Properties::Orientation::PITCH_DEG, m_fdm->GetPropertyValue("attitude/theta-deg"));
    local.set(Properties::Orientation::ROLL_DEG, m_fdm->GetPropertyValue("attitude/phi-deg"));
    local.set(Properties::Orientation::HEADING_DEG, m_fdm->GetPropertyValue("attitude/psi-deg"));
    local.set(Properties::Velocities::VERTICAL_SPEED_FPS, m_fdm->GetPropertyValue("velocities/v-down-fps") * -1.0);
}

void JsbsimSystem::update(float dt) {
    ensureInitialized(dt);
    if (!m_initialized) {
        return;
    }

    m_fdm->Setdt(dt);
    if (m_config.terrain) {
        const_cast<TerrainRenderer*>(m_config.terrain)
            ->preloadPhysicsAt(m_acState->position.x, m_acState->position.z, 1);
    }
    syncInputs();
    m_fdm->Run();
    syncOutputs();
}

} // namespace nuage
