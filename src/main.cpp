#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "math/vec3.hpp"
#include "math/mat4.hpp"
#include "graphics/shader.hpp"
#include "game/terrain.hpp"
#include "game/aircraft.hpp"

using namespace flightsim;

namespace {

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

const char* VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vColor;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* FRAGMENT_SHADER = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Flight Simulator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);

    Shader shader;
    if (!shader.init(VERTEX_SHADER, FRAGMENT_SHADER)) {
        return -1;
    }
    GLint mvpLoc = shader.getUniformLocation("uMVP");

    Terrain terrain;
    terrain.init(100, 10.0f);

    Aircraft aircraft;
    aircraft.init();

    Mat4 projection = Mat4::perspective(3.14159f / 3.0f, 
        static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 2000.0f);

    float lastTime = glfwGetTime();

    std::cout << "Controls: W/S - Pitch, A/D - Yaw, Space - Accelerate, Shift - Decelerate, ESC - Quit" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        bool up = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool down = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        bool left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool accel = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool decel = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                     glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

        aircraft.update(dt, up, down, left, right, accel, decel);

        Vec3 pos = aircraft.getPosition();
        Vec3 camOffset = aircraft.getForward() * (-25.0f) + Vec3(0, 10, 0);
        Vec3 camPos = pos + camOffset;
        Mat4 view = Mat4::lookAt(camPos, pos, Vec3(0, 1, 0));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();

        Mat4 mvpTerrain = projection * view;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvpTerrain.m);
        terrain.draw();

        Mat4 mvpAircraft = projection * view * aircraft.getModelMatrix();
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvpAircraft.m);
        aircraft.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
