#include "core/app.hpp"
#include "ui/anchor.hpp"
#include "graphics/glad.h"
#include "graphics/mesh_builder.hpp"
#include "aircraft/aircraft.hpp"
#include "utils/config_loader.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <string>
#include <chrono>
#include <iomanip>
#include <filesystem>

namespace nuage {

bool App::init(const AppConfig& config) {
    if (!initWindow(config)) return false;

    m_input.init(m_window);
    m_assets.loadShader("basic", "assets/shaders/basic.vert", "assets/shaders/basic.frag");
    m_assets.loadShader("textured", "assets/shaders/textured.vert", "assets/shaders/textured.frag");
    m_assets.loadShader("sky", "assets/shaders/sky.vert", "assets/shaders/sky.frag");

    glGenVertexArrays(1, &m_skyVao);
    
    m_atmosphere.init();
    m_aircraft.init(m_assets, m_atmosphere);
    m_camera.init(m_input);
    m_scenery.init(m_assets);
    
    m_camera.setAspect(static_cast<float>(config.windowWidth) / config.windowHeight);

    if (!m_ui.init(this)) {
        std::cerr << "Failed to initialize UI system" << std::endl;
        return false;
    }

    setupScene();
    setupHUD();

    m_lastFrameTime = static_cast<float>(glfwGetTime());
    return true;
}

void App::run() {
    using clock = std::chrono::high_resolution_clock;

    while (!m_shouldQuit && !glfwWindowShouldClose(m_window)) {
        auto frameStart = clock::now();

        float now = static_cast<float>(glfwGetTime());
        m_deltaTime = now - m_lastFrameTime;
        m_lastFrameTime = now;
        m_time = now;

        auto inputStart = clock::now();
        m_input.update(m_deltaTime);
        auto inputEnd = clock::now();

        if (m_input.isKeyPressed(GLFW_KEY_TAB)) {
            m_camera.toggleOrbitMode();
        }

        if (m_input.quitRequested()) {
            m_shouldQuit = true;
            continue;
        }

        auto physicsStart = clock::now();
        updatePhysics();
        auto physicsEnd = clock::now();
        
        float alpha = m_physicsAccumulator / FIXED_DT;

        auto atmosphereStart = clock::now();
        m_atmosphere.update(m_deltaTime);
        auto atmosphereEnd = clock::now();
        auto cameraStart = clock::now();
        m_camera.update(m_deltaTime, m_aircraft.player(), alpha);
        auto cameraEnd = clock::now();

        auto hudStart = clock::now();
        updateHUD();
        auto hudEnd = clock::now();
        auto renderStart = clock::now();
        render(alpha);
        auto renderEnd = clock::now();

        auto swapStart = clock::now();
        glfwSwapBuffers(m_window);
        glfwPollEvents();
        auto swapEnd = clock::now();

        auto toMs = [](const clock::time_point& start, const clock::time_point& end) {
            return std::chrono::duration<double, std::milli>(end - start).count();
        };

        FrameProfile profile;
        profile.frameMs = static_cast<float>(toMs(frameStart, swapEnd));
        profile.inputMs = static_cast<float>(toMs(inputStart, inputEnd));
        profile.physicsMs = static_cast<float>(toMs(physicsStart, physicsEnd));
        profile.atmosphereMs = static_cast<float>(toMs(atmosphereStart, atmosphereEnd));
        profile.cameraMs = static_cast<float>(toMs(cameraStart, cameraEnd));
        profile.hudMs = static_cast<float>(toMs(hudStart, hudEnd));
        profile.renderMs = static_cast<float>(toMs(renderStart, renderEnd));
        profile.swapMs = static_cast<float>(toMs(swapStart, swapEnd));

        updateFrameStats(profile);
    }
}

void App::shutdown() {
    m_aircraft.shutdown();
    m_ui.shutdown();
    m_assets.unloadAll();
    if (m_skyVao) {
        glDeleteVertexArrays(1, &m_skyVao);
        m_skyVao = 0;
    }
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool App::initWindow(const AppConfig& config) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    m_window = glfwCreateWindow(config.windowWidth, config.windowHeight,
                                 config.title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    if (config.vsync) glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
    
    return true;
}

void App::setupScene() {
    m_terrainShader = m_assets.getShader("basic");
    m_terrainTexturedShader = m_assets.getShader("textured");
    m_terrainTextured = false;
    m_terrainTexture = nullptr;

    std::filesystem::path terrainConfigPath;
    {
        std::filesystem::path cwd = std::filesystem::current_path();
        std::filesystem::path probe = cwd;
        for (int i = 0; i < 6; ++i) {
            std::filesystem::path candidate = probe / "assets/config/terrain.json";
            if (std::filesystem::exists(candidate)) {
                terrainConfigPath = candidate;
                break;
            }
            if (probe.has_parent_path()) {
                probe = probe.parent_path();
            } else {
                break;
            }
        }
    }
    if (terrainConfigPath.empty()) {
        terrainConfigPath = "assets/config/terrain.json";
    }

    auto resolvePath = [&](const std::string& rawPath) {
        if (rawPath.empty()) return rawPath;
        std::filesystem::path p(rawPath);
        if (p.is_absolute()) return rawPath;
        return (terrainConfigPath.parent_path() / p).string();
    };

    auto terrainConfigOpt = loadJsonConfig(terrainConfigPath.string());
    if (terrainConfigOpt && terrainConfigOpt->contains("heightmap")) {
        const auto& config = *terrainConfigOpt;
        std::string heightmap = resolvePath(config.value("heightmap", ""));
        float sizeX = config.value("sizeX", 20000.0f);
        float sizeZ = config.value("sizeZ", 20000.0f);
        float heightMin = config.value("heightMin", 0.0f);
        float heightMax = config.value("heightMax", 1000.0f);
        int maxResolution = config.value("maxResolution", 512);
        bool flipY = config.value("flipY", true);
        std::string albedo = resolvePath(config.value("albedo", ""));

        bool textureLoaded = false;
        if (!albedo.empty()) {
            textureLoaded = m_assets.loadTexture("terrain_albedo", albedo);
            if (textureLoaded) {
                m_terrainTexture = m_assets.getTexture("terrain_albedo");
            } else {
                std::cerr << "Failed to load terrain albedo: " << albedo << std::endl;
            }
        }

        bool useTexture = textureLoaded && m_terrainTexture;
        auto terrainData = MeshBuilder::terrainFromHeightmap(heightmap,
                                                             sizeX,
                                                             sizeZ,
                                                             heightMin,
                                                             heightMax,
                                                             maxResolution,
                                                             useTexture,
                                                             flipY);
        if (!terrainData.empty()) {
            if (useTexture) {
                m_assets.loadTexturedMesh("terrain", terrainData);
                m_terrainTextured = true;
            } else {
                m_assets.loadMesh("terrain", terrainData);
            }
        } else {
            std::cerr << "Failed to build terrain from heightmap: " << heightmap << std::endl;
        }
    }

    if (!m_assets.getMesh("terrain")) {
        auto terrainData = MeshBuilder::terrain(20000.0f, 40);
        m_assets.loadMesh("terrain", terrainData);
    }

    m_terrainMesh = m_assets.getMesh("terrain");
    m_scenery.loadConfig("assets/config/scenery.json");
}

void App::setupHUD() {
    auto setupText = [](Text& text, float x, float y, float scale, Anchor anchor, float padding) {
        text.scaleVal(scale);
        text.pos(x, y);
        text.colorR(1, 1, 1);
        text.anchorMode(anchor);
        text.paddingValue(padding);
    };

    m_altitudeText = &m_ui.text("ALT: 0.0 m");
    setupText(*m_altitudeText, 20, 20, 2.0f, Anchor::TopLeft, 0.0f);

    m_airspeedText = &m_ui.text("SPD: 0 kts");
    setupText(*m_airspeedText, 20, 70, 2.0f, Anchor::TopLeft, 0.0f);

    m_headingText = &m_ui.text("HDG: 000");
    setupText(*m_headingText, 20, 120, 2.0f, Anchor::TopLeft, 0.0f);

    m_positionText = &m_ui.text("POS: 0, 0, 0");
    setupText(*m_positionText, 20, 170, 2.0f, Anchor::TopLeft, 0.0f);

}

void App::updatePhysics() {
    m_physicsAccumulator += m_deltaTime;
    while (m_physicsAccumulator >= FIXED_DT) {
        m_aircraft.fixedUpdate(FIXED_DT, m_input);
        m_physicsAccumulator -= FIXED_DT;
    }
}

void App::updateHUD() {
    Aircraft::Instance* player = m_aircraft.player();
    if (player && m_altitudeText && m_airspeedText && m_headingText && m_positionText) {
        Vec3 pos = player->position();
        float airspeed = player->airspeed();
        Vec3 fwd = player->forward();

        float heading = std::atan2(fwd.x, fwd.z) * 180.0f / 3.14159265f;
        if (heading < 0) heading += 360.0f;

        float airspeedKnots = airspeed * 1.94384f;

        auto fmt = [](float val) {
            std::string s = std::to_string(val);
            return s.substr(0, s.find('.') + 2);
        };

        m_altitudeText->content("ALT: " + fmt(pos.y) + " m");
        m_airspeedText->content("SPD: " + fmt(airspeedKnots) + " kts");
        m_headingText->content("HDG: " + std::to_string(static_cast<int>(heading)));
        m_positionText->content("POS: " + std::to_string(static_cast<int>(pos.x)) + ", " +
                                std::to_string(static_cast<int>(pos.y)) + ", " +
                                std::to_string(static_cast<int>(pos.z)));
    }

    m_ui.update();
}

void App::render(float alpha) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 vp = m_camera.viewProjection();

    if (!m_skyShader) {
        m_skyShader = m_assets.getShader("sky");
    }
    if (m_skyShader && m_skyVao) {
        Mat4 view = m_camera.viewMatrix();
        Vec3 right(view.m[0], view.m[4], view.m[8]);
        Vec3 up(view.m[1], view.m[5], view.m[9]);
        Vec3 forward(-view.m[2], -view.m[6], -view.m[10]);

        right = right.normalize();
        up = up.normalize();
        forward = forward.normalize();

        Mat4 proj = m_camera.projectionMatrix();
        float tanHalfFov = (proj.m[5] != 0.0f) ? (1.0f / proj.m[5]) : 1.0f;
        float aspect = (proj.m[0] != 0.0f) ? (proj.m[5] / proj.m[0]) : 1.0f;

        Vec3 sunDir = m_atmosphere.getSunDirection();

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_skyShader->use();
        m_skyShader->setVec3("uCameraRight", right);
        m_skyShader->setVec3("uCameraUp", up);
        m_skyShader->setVec3("uCameraForward", forward);
        m_skyShader->setFloat("uAspect", aspect);
        m_skyShader->setFloat("uTanHalfFov", tanHalfFov);
        m_skyShader->setVec3("uSunDir", sunDir);
        glBindVertexArray(m_skyVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }

    if (m_terrainMesh && m_terrainShader) {
        Shader* terrainShader = m_terrainShader;
        if (m_terrainTextured && m_terrainTexturedShader && m_terrainTexture) {
            terrainShader = m_terrainTexturedShader;
            terrainShader->use();
            terrainShader->setMat4("uMVP", vp);
            m_terrainTexture->bind(0);
            terrainShader->setInt("uTexture", 0);
        } else {
            terrainShader->use();
            terrainShader->setMat4("uMVP", vp);
        }
        m_terrainMesh->draw();
    }
    
    m_scenery.render(vp);
    
    m_aircraft.render(vp, alpha);
    
    m_ui.draw();
}

void App::updateFrameStats(const FrameProfile& profile) {
    m_framesSinceFps++;
    m_totalFrames++;
    m_fpsTimer += m_deltaTime;

    m_profileAccum.frames++;
    m_profileAccum.frameMs += profile.frameMs;
    m_profileAccum.inputMs += profile.inputMs;
    m_profileAccum.physicsMs += profile.physicsMs;
    m_profileAccum.atmosphereMs += profile.atmosphereMs;
    m_profileAccum.cameraMs += profile.cameraMs;
    m_profileAccum.hudMs += profile.hudMs;
    m_profileAccum.renderMs += profile.renderMs;
    m_profileAccum.swapMs += profile.swapMs;

    if (m_fpsTimer < 1.0f) return;

    double invFrames = (m_profileAccum.frames > 0) ? (1.0 / m_profileAccum.frames) : 0.0;
    m_lastFps = static_cast<float>(m_framesSinceFps) / m_fpsTimer;
    m_lastProfile.frameMs = static_cast<float>(m_profileAccum.frameMs * invFrames);
    m_lastProfile.inputMs = static_cast<float>(m_profileAccum.inputMs * invFrames);
    m_lastProfile.physicsMs = static_cast<float>(m_profileAccum.physicsMs * invFrames);
    m_lastProfile.atmosphereMs = static_cast<float>(m_profileAccum.atmosphereMs * invFrames);
    m_lastProfile.cameraMs = static_cast<float>(m_profileAccum.cameraMs * invFrames);
    m_lastProfile.hudMs = static_cast<float>(m_profileAccum.hudMs * invFrames);
    m_lastProfile.renderMs = static_cast<float>(m_profileAccum.renderMs * invFrames);
    m_lastProfile.swapMs = static_cast<float>(m_profileAccum.swapMs * invFrames);

    m_framesSinceFps = 0;
    m_fpsTimer = 0.0f;
    m_profileAccum = {};
}

}
