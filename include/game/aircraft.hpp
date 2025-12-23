#pragma once

#include "math/vec3.hpp"
#include "math/mat4.hpp"
#include "graphics/mesh.hpp"
#include <vector>

namespace flightsim {

class Aircraft {
public:
    void init();
    void update(float dt, bool up, bool down, bool left, bool right, bool accel, bool decel);
    void draw() const;

    Mat4 getModelMatrix() const;
    Vec3 getPosition() const { return m_pos; }
    Vec3 getForward() const;

private:
    std::vector<float> generateVertices();

    Vec3 m_pos{0, 30, 0};
    float m_yaw = 0;
    float m_pitch = 0;
    float m_speed = 20.0f;
    Mesh m_mesh;
};

}
