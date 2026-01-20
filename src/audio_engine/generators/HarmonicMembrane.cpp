#include "HarmonicMembrane.h"
#include <cmath>
#include <algorithm>

namespace KickDrum {

// Mathematical constant for 2*PI
constexpr float TWO_PI = 6.28318530717958647692f;

// Valid ratio range
constexpr float MIN_RATIO = 0.5f;
constexpr float MAX_RATIO = 8.0f;

HarmonicMembrane::HarmonicMembrane()
    : sampleRate_(0.0f)
    , baseFrequency_(0.0f)
    , ratio_(1.0f)
    , phase_(0.0f)
    , phaseIncrement_(0.0f)
    , initialized_(false)
{
}

void HarmonicMembrane::initialize(float sampleRate) {
    if (sampleRate <= 0.0f) {
        return; // Invalid sample rate
    }
    
    sampleRate_ = sampleRate;
    initialized_ = true;
    
    // Update phase increment if frequency was already set
    if (baseFrequency_ > 0.0f) {
        updatePhaseIncrement();
    }
}

void HarmonicMembrane::setBaseFrequency(float frequency) {
    if (frequency < 0.0f) {
        frequency = 0.0f; // Clamp to non-negative
    }
    
    baseFrequency_ = frequency;
    
    if (initialized_) {
        updatePhaseIncrement();
    }
}

float HarmonicMembrane::getBaseFrequency() const {
    return baseFrequency_;
}

void HarmonicMembrane::setRatio(float ratio) {
    // Clamp ratio to valid range [0.5, 8.0]
    ratio_ = std::clamp(ratio, MIN_RATIO, MAX_RATIO);
    
    if (initialized_) {
        updatePhaseIncrement();
    }
}

float HarmonicMembrane::getRatio() const {
    return ratio_;
}

float HarmonicMembrane::getFrequency() const {
    return baseFrequency_ * ratio_;
}

void HarmonicMembrane::reset() {
    phase_ = 0.0f;
}

float HarmonicMembrane::generate() {
    if (!initialized_) {
        return 0.0f; // Return silence if not initialized
    }
    
    // Generate sine wave sample using current phase
    // Phase is in range [0.0, 1.0], so multiply by 2*PI for radians
    float sample = std::sin(phase_ * TWO_PI);
    
    // Advance phase
    phase_ += phaseIncrement_;
    
    // Wrap phase to [0.0, 1.0) range
    // Using fmod for precise wrapping
    if (phase_ >= 1.0f) {
        phase_ -= std::floor(phase_);
    }
    
    return sample;
}

bool HarmonicMembrane::isInitialized() const {
    return initialized_;
}

void HarmonicMembrane::updatePhaseIncrement() {
    if (sampleRate_ > 0.0f) {
        // Phase increment = (baseFrequency × ratio) / sampleRate
        // This gives us the fraction of a cycle to advance per sample
        float actualFrequency = baseFrequency_ * ratio_;
        phaseIncrement_ = actualFrequency / sampleRate_;
    } else {
        phaseIncrement_ = 0.0f;
    }
}

} // namespace KickDrum
