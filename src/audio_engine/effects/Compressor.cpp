#include "Compressor.h"
#include <algorithm> // for std::clamp
#include <cmath>

namespace KickDrum {

// Small value to prevent log(0) and division by zero
constexpr float EPSILON = 1e-10f;

// Minimum dB value to prevent -infinity
constexpr float MIN_DB = -96.0f;

Compressor::Compressor()
    : sampleRate_(0.0f)
    , thresholdDb_(-12.0f)      // Default: -12dB threshold
    , ratio_(4.0f)               // Default: 4:1 compression
    , attackSeconds_(0.005f)     // Default: 5ms attack
    , releaseSeconds_(0.1f)      // Default: 100ms release
    , mix_(1.0f)                 // Default: fully wet
    , envelopeDb_(0.0f)          // Start with no gain reduction
    , attackCoeff_(0.0f)
    , releaseCoeff_(0.0f)
    , initialized_(false)
{
}

void Compressor::initialize(float sampleRate) {
    if (sampleRate <= 0.0f) {
        return; // Invalid sample rate
    }
    
    sampleRate_ = sampleRate;
    initialized_ = true;
    
    // Update coefficients based on current attack/release times
    updateCoefficients();
}

void Compressor::setThreshold(float thresholdDb) {
    // Clamp to reasonable range (-60dB to 0dB)
    thresholdDb_ = std::clamp(thresholdDb, -60.0f, 0.0f);
}

float Compressor::getThreshold() const {
    return thresholdDb_;
}

void Compressor::setRatio(float ratio) {
    // Clamp to valid range (1.0 to 20.0)
    ratio_ = std::clamp(ratio, 1.0f, 20.0f);
}

float Compressor::getRatio() const {
    return ratio_;
}

void Compressor::setAttack(float attackSeconds) {
    // Clamp to reasonable range (0.1ms to 100ms)
    attackSeconds_ = std::clamp(attackSeconds, 0.0001f, 0.1f);
    
    if (initialized_) {
        updateCoefficients();
    }
}

float Compressor::getAttack() const {
    return attackSeconds_;
}

void Compressor::setRelease(float releaseSeconds) {
    // Clamp to reasonable range (10ms to 1000ms)
    releaseSeconds_ = std::clamp(releaseSeconds, 0.01f, 1.0f);
    
    if (initialized_) {
        updateCoefficients();
    }
}

float Compressor::getRelease() const {
    return releaseSeconds_;
}

void Compressor::setMix(float mix) {
    // Clamp to valid range [0.0, 1.0]
    mix_ = std::clamp(mix, 0.0f, 1.0f);
}

float Compressor::getMix() const {
    return mix_;
}

float Compressor::process(float input) {
    if (!initialized_) {
        return input; // Bypass if not initialized
    }
    
    // Step 1: Convert input to dB
    float inputDb = linearToDb(std::abs(input));
    
    // Step 2: Calculate target gain reduction
    float targetGainReductionDb = 0.0f;
    
    if (inputDb > thresholdDb_) {
        // Calculate how much the signal exceeds the threshold
        float overDb = inputDb - thresholdDb_;
        
        // Apply compression ratio
        // gainReduction = over × (1 - 1/ratio)
        // For ratio = 1.0: gainReduction = 0 (no compression)
        // For ratio = 2.0: gainReduction = over × 0.5 (2:1 compression)
        // For ratio = inf: gainReduction = over (limiting)
        targetGainReductionDb = overDb * (1.0f - 1.0f / ratio_);
    }
    
    // Step 3: Smooth gain reduction with attack/release ballistics
    // Use different coefficients depending on whether we're attacking or releasing
    float coeff;
    if (targetGainReductionDb > envelopeDb_) {
        // Signal is getting louder, use attack coefficient
        coeff = attackCoeff_;
    } else {
        // Signal is getting quieter, use release coefficient
        coeff = releaseCoeff_;
    }
    
    // Apply exponential smoothing
    envelopeDb_ = envelopeDb_ + coeff * (targetGainReductionDb - envelopeDb_);
    
    // Step 4: Apply gain reduction
    float gainReductionLinear = dbToLinear(-envelopeDb_);
    float compressed = input * gainReductionLinear;
    
    // Step 5: Mix dry and wet signals
    float output = input * (1.0f - mix_) + compressed * mix_;
    
    return output;
}

void Compressor::reset() {
    envelopeDb_ = 0.0f;
}

bool Compressor::isInitialized() const {
    return initialized_;
}

float Compressor::getGainReduction() const {
    return envelopeDb_;
}

void Compressor::updateCoefficients() {
    if (sampleRate_ <= 0.0f) {
        return;
    }
    
    // Calculate attack and release coefficients for exponential smoothing
    // The coefficient determines how quickly the envelope follows the target
    // 
    // For a time constant tau (in seconds), the coefficient is:
    //   coeff = 1 - exp(-1 / (tau × sampleRate))
    // 
    // This gives approximately 63% of the way to the target in tau seconds
    
    if (attackSeconds_ > 0.0f) {
        attackCoeff_ = 1.0f - std::exp(-1.0f / (attackSeconds_ * sampleRate_));
    } else {
        attackCoeff_ = 1.0f; // Instant attack
    }
    
    if (releaseSeconds_ > 0.0f) {
        releaseCoeff_ = 1.0f - std::exp(-1.0f / (releaseSeconds_ * sampleRate_));
    } else {
        releaseCoeff_ = 1.0f; // Instant release
    }
}

float Compressor::linearToDb(float linear) {
    if (linear < EPSILON) {
        return MIN_DB; // Return minimum dB instead of -infinity
    }
    return 20.0f * std::log10(linear);
}

float Compressor::dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

} // namespace KickDrum
