#include "VoiceAllocator.h"

#include <algorithm>

namespace KickDrum {

VoiceAllocator::VoiceAllocator()
    : params_(kDefaultKickParams)
    , sampleRate_(48000.0f)
    , initialized_(false) {
}

void VoiceAllocator::initialize(float sampleRate) {
    sampleRate_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
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
        } else {
            voice.setParams(params_);
        }
    }
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
