#include "SineDriver.h"
#include <cmath>

namespace KickDrum {

// Mathematical constant for 2*PI
constexpr float TWO_PI = 6.28318530717958647692f;

SineDriver::SineDriver()
    : sampleRate_(0.0f)
    , frequency_(0.0f)
    , phase_(0.0f)
    , phaseIncrement_(0.0f)
    , initialized_(false)
{
}

void SineDriver::initialize(float sampleRate) {
    if (sampleRate <= 0.0f) {
        return; // Invalid sample rate
    }
    
    sampleRate_ = sampleRate;
    initialized_ = true;
    
    // Update phase increment if frequency was already set
    if (frequency_ > 0.0f) {
        updatePhaseIncrement();
    }
}

void SineDriver::setFrequency(float frequency) {
    if (frequency < 0.0f) {
        frequency = 0.0f; // Clamp to non-negative
    }
    
    frequency_ = frequency;
    
    if (initialized_) {
        updatePhaseIncrement();
    }
}

float SineDriver::getFrequency() const {
    return frequency_;
}

void SineDriver::reset() {
    phase_ = 0.0f;
}

void SineDriver::setPhase(float phase) {
    phase_ = phase - std::floor(phase);
    if (phase_ < 0.0f) {
        phase_ += 1.0f;
    }
}

float SineDriver::getPhase() const {
    return phase_;
}

float SineDriver::generate() {
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

bool SineDriver::isInitialized() const {
    return initialized_;
}

void SineDriver::updatePhaseIncrement() {
    if (sampleRate_ > 0.0f) {
        // Phase increment = frequency / sampleRate
        // This gives us the fraction of a cycle to advance per sample
        phaseIncrement_ = frequency_ / sampleRate_;
    } else {
        phaseIncrement_ = 0.0f;
    }
}

} // namespace KickDrum
