#pragma once

#include "core/subsystem.hpp"
#include <memory>
#include <string>

namespace nuage {

class AudioSubsystem : public Subsystem {
public:
    AudioSubsystem();
    ~AudioSubsystem() override;

    void init() override;
    void update(double dt) override;
    void shutdown() override;

    std::string getName() const override { return "Audio"; }

    void setEngineSoundPath(const std::string& path) { m_engineSoundPath = path; }

private:
    struct Impl;

    std::unique_ptr<Impl> m_impl;
    std::string m_engineSoundPath = "assets/c172p.wav";

    bool m_ready = false;
    float m_gain = 0.0f;
    float m_minGain = 0.15f;
    float m_maxGain = 1.0f;
    float m_gainResponse = 4.0f;
};

} // namespace nuage
