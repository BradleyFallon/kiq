#include "VoiceAllocator.h"
#include <algorithm>
#include <limits>

namespace KickDrum {

VoiceAllocator::VoiceAllocator()
    : sampleRate_(44100.0f)
    , initialized_(false)
{
    // Create voice pool with MAX_POLYPHONY voices
    voices_.resize(MAX_POLYPHONY);
}

void VoiceAllocator::initialize(float sampleRate) {
    sampleRate_ = sampleRate;
    
    // Initialize all voices with the sample rate
    for (auto& voice : voices_) {
        voice.initialize(sampleRate);
    }
    
    initialized_ = true;
}

Voice* VoiceAllocator::allocateVoice(int note, float velocity) {
    if (!initialized_) {
        return nullptr;
    }
    
    Voice* voice = nullptr;
    
    // Step 1: Try to find an idle voice
    voice = findIdleVoice();
    
    // Step 2: If no idle voice, steal the oldest voice
    if (voice == nullptr) {
        voice = findOldestVoice();
    }
    
    // Step 3: Trigger the allocated voice
    if (voice != nullptr) {
        voice->trigger(note, velocity);
    }
    
    return voice;
}

void VoiceAllocator::releaseVoice(int note) {
    // Find the voice playing this note
    Voice* voice = findVoiceByNote(note);
    
    if (voice != nullptr) {
        voice->release();
    }
}

void VoiceAllocator::releaseAll() {
    // Release all active voices
    for (auto& voice : voices_) {
        if (voice.isActive()) {
            voice.release();
        }
    }
}

void VoiceAllocator::renderBuffer(float* buffer, int numSamples) {
    if (!initialized_ || buffer == nullptr || numSamples <= 0) {
        return;
    }
    
    // Clear the buffer first
    for (int i = 0; i < numSamples; ++i) {
        buffer[i] = 0.0f;
    }
    
    // Mix all active voices into the buffer
    for (auto& voice : voices_) {
        if (voice.isActive()) {
            for (int i = 0; i < numSamples; ++i) {
                buffer[i] += voice.renderSample();
            }
        }
    }
}

int VoiceAllocator::getNumActiveVoices() const {
    int count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void VoiceAllocator::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    
    // Update all voices with new sample rate
    for (auto& voice : voices_) {
        voice.setSampleRate(sampleRate);
    }
}

void VoiceAllocator::setPitchTrackingEnabled(bool enabled) {
    // Update all voices with pitch tracking state
    for (auto& voice : voices_) {
        voice.setPitchTrackingEnabled(enabled);
    }
}

Voice* VoiceAllocator::findIdleVoice() {
    // Find the first inactive voice
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

Voice* VoiceAllocator::findOldestVoice() {
    // Find the voice with the highest age (oldest)
    // If all voices have the same age, return the first one
    Voice* oldestVoice = &voices_[0];  // Start with first voice
    uint64_t maxAge = voices_[0].getAge();
    
    for (size_t i = 1; i < voices_.size(); ++i) {
        if (voices_[i].getAge() > maxAge) {
            maxAge = voices_[i].getAge();
            oldestVoice = &voices_[i];
        }
    }
    
    return oldestVoice;
}

Voice* VoiceAllocator::findVoiceByNote(int note) {
    // Find the first voice playing this note
    // If multiple voices are playing the same note, return the oldest one
    Voice* foundVoice = nullptr;
    uint64_t maxAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            if (voice.getAge() > maxAge) {
                maxAge = voice.getAge();
                foundVoice = &voice;
            }
        }
    }
    
    return foundVoice;
}

} // namespace KickDrum
