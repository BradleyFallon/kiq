#include "VoiceAllocator.h"

#include <algorithm>
#include <cmath>

namespace KickDrum {

VoiceAllocator::VoiceAllocator()
    : params_(kDefaultKickParams)
    , sampleLayer_(nullptr)
    , sampleLayerRevision_(0)
    , sampleRate_(48000.0f)
    , initialized_(false) {
}

void VoiceAllocator::setSampleLayer(const SampleLayerData* sampleLayer,
                                    std::uint64_t revision) {
    // Active hits retain the pointer/revision captured at trigger. AudioEngine
    // reclaims retired immutable buffers only after all such revisions finish.
    sampleLayer_ = sampleLayer;
    sampleLayerRevision_ = sampleLayer ? revision : 0;
}

void VoiceAllocator::initialize(float sampleRate) {
    sampleRate_ = std::isfinite(sampleRate) && sampleRate > 0.0f
                      ? sampleRate
                      : 48000.0f;
    for (auto& voice : voices_) {
        voice.initialize(sampleRate_);
        voice.setParams(params_);
    }
    initialized_ = true;
}

void VoiceAllocator::setSampleRate(float sampleRate) {
    initialize(sampleRate);
}

void VoiceAllocator::setParams(const KickParams& params) {
    params_ = sanitizeKickParams(params);
    for (auto& voice : voices_) {
        if (voice.isActive()) {
            // A kick is a physical event: keep its trajectory, membrane, and
            // transient snapshot stable for the rest of that hit. Output is
            // the one live control and is ramped inside Voice to avoid clicks.
            voice.setOutputGain(params_.outputGain);
        }
    }

    // Idle voices are configured exactly once when allocated below. Rebuilding
    // every idle membrane and phase-lock trajectory at every automation point
    // is redundant and can make dense host automation miss realtime deadlines.
}

void VoiceAllocator::setOutputStageParams(const OutputStageParams& params) {
    params_.outputStage = params;
}

Voice* VoiceAllocator::allocateVoice(int note, float velocity) {
    if (!initialized_) {
        return nullptr;
    }

    Voice* voice = findIdleVoice();
    if (!voice) {
        voice = findOldestVoice();
    }
    if (voice) {
        voice->setParams(params_);
        voice->setSampleLayer(sampleLayer_, sampleLayerRevision_);
        voice->trigger(note, velocity);
    }
    return voice;
}

void VoiceAllocator::releaseVoice(int) {
    // Kick hits are one-shots; note-off intentionally does not truncate them.
}

void VoiceAllocator::releaseAll() {
    for (auto& voice : voices_) {
        voice.stop();
    }
}

void VoiceAllocator::renderBuffer(float* buffer, int numSamples) {
    if (!buffer || numSamples <= 0) {
        return;
    }
    std::fill(buffer, buffer + numSamples, 0.0f);
    if (!initialized_) {
        return;
    }

    for (auto& voice : voices_) {
        for (int sample = 0; sample < numSamples && voice.isActive(); ++sample) {
            buffer[sample] += voice.renderSample();
        }
    }
}

int VoiceAllocator::getNumActiveVoices() const {
    return static_cast<int>(std::count_if(
        voices_.begin(), voices_.end(), [](const Voice& voice) { return voice.isActive(); }));
}

std::array<std::uint64_t, VoiceAllocator::kMaxVoices>
VoiceAllocator::activeSampleLayerRevisions() const {
    std::array<std::uint64_t, kMaxVoices> revisions {};
    for (std::size_t index = 0; index < voices_.size(); ++index) {
        revisions[index] = voices_[index].getActiveSampleLayerRevision();
    }
    return revisions;
}

Voice* VoiceAllocator::findIdleVoice() {
    const auto found = std::find_if(
        voices_.begin(), voices_.end(), [](const Voice& voice) { return !voice.isActive(); });
    return found == voices_.end() ? nullptr : &*found;
}

Voice* VoiceAllocator::findOldestVoice() {
    return &*std::max_element(voices_.begin(), voices_.end(),
                              [](const Voice& left, const Voice& right) {
                                  return left.getAge() < right.getAge();
                              });
}

} // namespace KickDrum
