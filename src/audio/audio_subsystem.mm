#include "audio/audio_subsystem.hpp"
#include "core/properties/property_bus.hpp"
#include "core/properties/property_paths.hpp"

#if defined(__APPLE__)
#import <AVFoundation/AVFoundation.h>
#endif

#include <algorithm>
#include <iostream>

namespace nuage {

#if defined(__APPLE__)

struct AudioSubsystem::Impl {
    AVAudioEngine* engine = nil;
    AVAudioPlayerNode* player = nil;
    AVAudioPCMBuffer* buffer = nil;
};

static void logError(const char* message, NSError* error) {
    std::cerr << "[Audio] " << message;
    if (error) {
        std::cerr << ": " << [[error localizedDescription] UTF8String];
    }
    std::cerr << std::endl;
}

AudioSubsystem::AudioSubsystem() = default;
AudioSubsystem::~AudioSubsystem() = default;

void AudioSubsystem::init() {
    @autoreleasepool {
        m_impl = std::make_unique<Impl>();
        m_impl->engine = [[AVAudioEngine alloc] init];
        m_impl->player = [[AVAudioPlayerNode alloc] init];
        [m_impl->engine attachNode:m_impl->player];

        NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:m_engineSoundPath.c_str()]];
        NSError* error = nil;
        AVAudioFile* file = [[AVAudioFile alloc] initForReading:url error:&error];
        if (!file) {
            logError("Failed to open audio file", error);
            m_impl.reset();
            return;
        }

        AVAudioPCMBuffer* buffer = [[AVAudioPCMBuffer alloc]
            initWithPCMFormat:file.processingFormat
                 frameCapacity:(AVAudioFrameCount)file.length];
        if (![file readIntoBuffer:buffer error:&error]) {
            logError("Failed to read audio file", error);
            m_impl.reset();
            return;
        }
        m_impl->buffer = buffer;

        [m_impl->engine connect:m_impl->player
                             to:m_impl->engine.mainMixerNode
                         format:buffer.format];

        if (![m_impl->engine startAndReturnError:&error]) {
            logError("Failed to start AVAudioEngine", error);
            m_impl.reset();
            return;
        }

        [m_impl->player scheduleBuffer:buffer
                                 atTime:nil
                                options:AVAudioPlayerNodeBufferLoops
                      completionHandler:nil];
        m_impl->player.volume = m_gain;
        [m_impl->player play];
        m_ready = true;
    }
}

void AudioSubsystem::update(double dt) {
    if (!m_ready || !m_impl || !m_impl->player) return;

    bool paused = PropertyBus::global().get(Properties::Sim::PAUSED, false);
    bool muted = PropertyBus::global().get(Properties::Audio::MUTED, false);
    double throttle = PropertyBus::global().get(Properties::Controls::THROTTLE, 0.0);
    float clamped = std::clamp(static_cast<float>(throttle), 0.0f, 1.0f);

    float targetGain = (paused || muted) ? 0.0f : (m_minGain + (m_maxGain - m_minGain) * clamped);
    float smoothing = std::clamp(static_cast<float>(dt) * m_gainResponse, 0.0f, 1.0f);
    m_gain += (targetGain - m_gain) * smoothing;

    m_impl->player.volume = m_gain;
}

void AudioSubsystem::shutdown() {
    if (!m_impl) return;
    @autoreleasepool {
        if (m_impl->player) {
            [m_impl->player stop];
        }
        if (m_impl->engine) {
            [m_impl->engine stop];
        }
        m_impl.reset();
    }
    m_ready = false;
}

#else

struct AudioSubsystem::Impl {};

AudioSubsystem::AudioSubsystem() = default;
AudioSubsystem::~AudioSubsystem() = default;

void AudioSubsystem::init() {}
void AudioSubsystem::update(double /*dt*/) {}
void AudioSubsystem::shutdown() {}

#endif

} // namespace nuage
