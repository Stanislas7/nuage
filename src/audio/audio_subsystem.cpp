#include "audio/audio_subsystem.hpp"

#if !defined(__APPLE__)
#include <iostream>

namespace nuage {

struct AudioSubsystem::Impl {};

AudioSubsystem::AudioSubsystem() = default;
AudioSubsystem::~AudioSubsystem() = default;

void AudioSubsystem::init() {
    std::cerr << "[Audio] Audio subsystem not implemented for this platform" << std::endl;
}

void AudioSubsystem::update(double /*dt*/) {
}

void AudioSubsystem::shutdown() {
}

} // namespace nuage
#endif
