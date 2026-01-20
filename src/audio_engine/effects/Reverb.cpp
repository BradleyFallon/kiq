#include "Reverb.h"
#include <algorithm> // for std::clamp, std::fill

namespace KickDrum {

// ============================================================================
// CombFilter Implementation
// ============================================================================

Reverb::CombFilter::CombFilter()
    : bufferSize_(0)
    , bufferIndex_(0)
    , feedback_(0.0f)
    , damping_(0.0f)
    , filterState_(0.0f)
{
}

void Reverb::CombFilter::initialize(int bufferSize, float sampleRate) {
    bufferSize_ = bufferSize;
    buffer_.resize(bufferSize);
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    bufferIndex_ = 0;
    filterState_ = 0.0f;
}

void Reverb::CombFilter::setFeedback(float feedback) {
    feedback_ = feedback;
}

void Reverb::CombFilter::setDamping(float damping) {
    damping_ = damping;
}

float Reverb::CombFilter::process(float input) {
    if (buffer_.empty()) {
        return input;
    }

    // Read from delay line
    float output = buffer_[bufferIndex_];

    // Apply damping filter (one-pole lowpass) to the feedback signal
    // This simulates high-frequency absorption in the room
    filterState_ = output * (1.0f - damping_) + filterState_ * damping_;

    // Write to delay line with feedback
    buffer_[bufferIndex_] = input + filterState_ * feedback_;

    // Advance buffer index (circular buffer)
    bufferIndex_++;
    if (bufferIndex_ >= bufferSize_) {
        bufferIndex_ = 0;
    }

    return output;
}

void Reverb::CombFilter::reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    bufferIndex_ = 0;
    filterState_ = 0.0f;
}

// ============================================================================
// AllpassFilter Implementation
// ============================================================================

Reverb::AllpassFilter::AllpassFilter()
    : bufferSize_(0)
    , bufferIndex_(0)
{
}

void Reverb::AllpassFilter::initialize(int bufferSize) {
    bufferSize_ = bufferSize;
    buffer_.resize(bufferSize);
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    bufferIndex_ = 0;
}

float Reverb::AllpassFilter::process(float input) {
    if (buffer_.empty()) {
        return input;
    }

    // Read from delay line
    float bufferOut = buffer_[bufferIndex_];

    // Allpass filter equation:
    // output = -input + bufferOut + feedback * input
    float output = -input + bufferOut;

    // Write to delay line
    buffer_[bufferIndex_] = input + bufferOut * FEEDBACK;

    // Advance buffer index (circular buffer)
    bufferIndex_++;
    if (bufferIndex_ >= bufferSize_) {
        bufferIndex_ = 0;
    }

    return output;
}

void Reverb::AllpassFilter::reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    bufferIndex_ = 0;
}

// ============================================================================
// Reverb Implementation
// ============================================================================

Reverb::Reverb()
    : sampleRate_(0.0f)
    , roomSize_(0.5f)           // Default: medium room
    , decayTime_(1.0f)          // Default: 1 second decay
    , damping_(0.5f)            // Default: moderate damping
    , mix_(0.3f)                // Default: 30% wet
    , initialized_(false)
{
}

Reverb::~Reverb() {
    // Destructor - vectors will clean up automatically
}

void Reverb::initialize(float sampleRate) {
    if (sampleRate <= 0.0f) {
        return; // Invalid sample rate
    }

    sampleRate_ = sampleRate;

    // Initialize comb filters with scaled delay times
    for (int i = 0; i < NUM_COMBS; i++) {
        int delaySize = scaleDelayTime(COMB_TUNINGS[i]);
        combFilters_[i].initialize(delaySize, sampleRate);
    }

    // Initialize allpass filters with scaled delay times
    for (int i = 0; i < NUM_ALLPASSES; i++) {
        int delaySize = scaleDelayTime(ALLPASS_TUNINGS[i]);
        allpassFilters_[i].initialize(delaySize);
    }

    initialized_ = true;

    // Update filter parameters
    updateCombFilters();
}

void Reverb::setRoomSize(float roomSize) {
    roomSize_ = std::clamp(roomSize, 0.0f, 1.0f);
    if (initialized_) {
        updateCombFilters();
    }
}

float Reverb::getRoomSize() const {
    return roomSize_;
}

void Reverb::setDecayTime(float decaySeconds) {
    decayTime_ = std::clamp(decaySeconds, 0.1f, 10.0f);
    if (initialized_) {
        updateCombFilters();
    }
}

float Reverb::getDecayTime() const {
    return decayTime_;
}

void Reverb::setDamping(float damping) {
    damping_ = std::clamp(damping, 0.0f, 1.0f);
    if (initialized_) {
        updateCombFilters();
    }
}

float Reverb::getDamping() const {
    return damping_;
}

void Reverb::setMix(float mix) {
    mix_ = std::clamp(mix, 0.0f, 1.0f);
}

float Reverb::getMix() const {
    return mix_;
}

float Reverb::process(float input) {
    if (!initialized_) {
        return input; // Bypass if not initialized
    }

    // Scale input
    float scaledInput = input * FIXED_GAIN;

    // Process through parallel comb filters and sum
    float combSum = 0.0f;
    for (int i = 0; i < NUM_COMBS; i++) {
        combSum += combFilters_[i].process(scaledInput);
    }

    // Process through series allpass filters
    float allpassOut = combSum;
    for (int i = 0; i < NUM_ALLPASSES; i++) {
        allpassOut = allpassFilters_[i].process(allpassOut);
    }

    // Scale wet signal
    float wet = allpassOut * SCALE_WET;

    // Mix dry and wet signals
    float output = input * (1.0f - mix_) + wet * mix_;

    return output;
}

void Reverb::reset() {
    // Clear all comb filters
    for (int i = 0; i < NUM_COMBS; i++) {
        combFilters_[i].reset();
    }

    // Clear all allpass filters
    for (int i = 0; i < NUM_ALLPASSES; i++) {
        allpassFilters_[i].reset();
    }
}

bool Reverb::isInitialized() const {
    return initialized_;
}

void Reverb::updateCombFilters() {
    // Calculate feedback based on room size and decay time
    // The feedback determines how long the reverb tail lasts
    // 
    // For a given decay time T (time to decay by 60dB), the feedback is:
    //   feedback = 10^(-3 * delayTime / (T * sampleRate))
    // 
    // We use a simplified approach based on Freeverb's original algorithm:
    // - Room size directly affects feedback (larger room = more feedback)
    // - We scale room size to a usable feedback range

    float roomScaled = roomSize_ * SCALE_ROOM + OFFSET_ROOM;
    
    // Adjust feedback based on decay time
    // Longer decay time = higher feedback
    float decayFactor = std::clamp(decayTime_ / 5.0f, 0.2f, 1.0f);
    float feedback = roomScaled * decayFactor;
    
    // Clamp feedback to stable range (must be < 1.0 for stability)
    feedback = std::clamp(feedback, 0.0f, 0.98f);

    // Calculate damping coefficient
    float dampingScaled = damping_ * SCALE_DAMPING;

    // Update all comb filters
    for (int i = 0; i < NUM_COMBS; i++) {
        combFilters_[i].setFeedback(feedback);
        combFilters_[i].setDamping(dampingScaled);
    }
}

int Reverb::scaleDelayTime(int tuning) const {
    // Scale delay time based on sample rate
    // Original Freeverb tunings are for 44.1kHz
    // We scale proportionally for other sample rates
    
    if (sampleRate_ <= 0.0f) {
        return tuning;
    }

    float scale = sampleRate_ / 44100.0f;
    int scaledDelay = static_cast<int>(tuning * scale);
    
    // Ensure minimum delay of 1 sample
    return std::max(1, scaledDelay);
}

} // namespace KickDrum
