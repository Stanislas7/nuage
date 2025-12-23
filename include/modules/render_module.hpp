#pragma once

#include "core/module.hpp"
#include "math/vec3.hpp"

namespace flightsim {

class RenderModule : public Module {
public:
    const char* name() const override { return "Render"; }

    void init(Simulator* sim) override;
    void render() override;
    void shutdown() override {} // Added empty shutdown to satisfy interface if needed, though base has default

    void setClearColor(const Vec3& color) { m_clearColor = color; }
    void setWireframe(bool enabled) { m_wireframe = enabled; }

private:
    Vec3 m_clearColor{0.5f, 0.7f, 0.9f};
    bool m_wireframe = false;
};

}
