#include "DualPhaseEnvelope.h"
#include <algorithm>
#include <cmath>

namespace KickDrum {

DualPhaseEnvelope::DualPhaseEnvelope(float sampleRate)
    : warmUpDuration(0.0f)
    , warmUpStartFreq(10.0f)
    , warmUpAmplitude(0.5f)
    , attack(0.001f)
    , decay(0.5f)
    , sustain(0.0f)
    , releaseTime(0.1f)
    , attackCurve(CurveType::EXPONENTIAL)
    , decayCurve(CurveType::LOGARITHMIC)
    , releaseCurve(CurveType::LOGARITHMIC)
    , currentPhase(EnvelopePhase::IDLE)
    , phaseTime(0.0f)
    , sampleRate(sampleRate)
    , currentValue(0.0f)
{
}

// ============================================================================
// Warm-Up Phase Parameters
// ============================================================================

void DualPhaseEnvelope::setWarmUpDuration(float duration) {
    // Clamp to valid range (0ms to 100ms)
    warmUpDuration = std::clamp(duration, 0.0f, 0.1f);
}

void DualPhaseEnvelope::setWarmUpStartFrequency(float frequency) {
    // Clamp to valid range (5Hz to 50Hz)
    warmUpStartFreq = std::clamp(frequency, 5.0f, 50.0f);
}

void DualPhaseEnvelope::setWarmUpAmplitude(float amplitude) {
    // Clamp to valid range (0.0 to 1.0)
    warmUpAmplitude = std::clamp(amplitude, 0.0f, 1.0f);
}

// ============================================================================
// Transient/Decay Phase Parameters (ADSR)
// ============================================================================

void DualPhaseEnvelope::setAttack(float time) {
    // Ensure non-negative
    attack = std::max(0.0f, time);
}

void DualPhaseEnvelope::setDecay(float time) {
    // Ensure non-negative
    decay = std::max(0.0f, time);
}

void DualPhaseEnvelope::setSustain(float level) {
    // Clamp to valid range (0.0 to 1.0)
    sustain = std::clamp(level, 0.0f, 1.0f);
}

void DualPhaseEnvelope::setRelease(float time) {
    // Ensure non-negative
    releaseTime = std::max(0.0f, time);
}

// ============================================================================
// Curve Shaping
// ============================================================================

void DualPhaseEnvelope::setAttackCurve(CurveType curve) {
    attackCurve = curve;
}

void DualPhaseEnvelope::setDecayCurve(CurveType curve) {
    decayCurve = curve;
}

void DualPhaseEnvelope::setReleaseCurve(CurveType curve) {
    releaseCurve = curve;
}

// ============================================================================
// Envelope Control
// ============================================================================

void DualPhaseEnvelope::trigger() {
    // Start from beginning
    phaseTime = 0.0f;
    
    // Enter WARMUP phase if duration > 0, otherwise go directly to ATTACK
    if (warmUpDuration > 0.0f) {
        currentPhase = EnvelopePhase::WARMUP;
        currentValue = 0.0f;
    } else {
        currentPhase = EnvelopePhase::ATTACK;
        currentValue = 0.0f;
    }
}

void DualPhaseEnvelope::release() {
    // Transition to RELEASE phase from any active phase
    if (currentPhase != EnvelopePhase::IDLE) {
        currentPhase = EnvelopePhase::RELEASE;
        phaseTime = 0.0f;
        // currentValue remains at its current level for smooth transition
    }
}

void DualPhaseEnvelope::reset() {
    currentPhase = EnvelopePhase::IDLE;
    phaseTime = 0.0f;
    currentValue = 0.0f;
}

void DualPhaseEnvelope::advance() {
    if (currentPhase == EnvelopePhase::IDLE) {
        currentValue = 0.0f;
        return;
    }
    
    // Increment phase time
    phaseTime += 1.0f;
    
    // Update the envelope value
    updateValue();
    
    // Check for phase transitions
    transitionToNextPhase();
}

float DualPhaseEnvelope::getValue() const {
    return currentValue;
}

bool DualPhaseEnvelope::isActive() const {
    return currentPhase != EnvelopePhase::IDLE;
}

void DualPhaseEnvelope::setSampleRate(float newSampleRate) {
    sampleRate = newSampleRate;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void DualPhaseEnvelope::updateValue() {
    switch (currentPhase) {
        case EnvelopePhase::IDLE:
            currentValue = 0.0f;
            break;
            
        case EnvelopePhase::WARMUP: {
            // Calculate normalized time within warm-up phase
            float warmUpDurationSamples = warmUpDuration * sampleRate;
            float t = (warmUpDurationSamples > 0.0f) 
                ? (phaseTime / warmUpDurationSamples) 
                : 1.0f;
            t = std::clamp(t, 0.0f, 1.0f);
            
            // Apply linear curve for warm-up (smooth sweep)
            // The warm-up phase builds gradually from 0 to warmUpAmplitude
            currentValue = t * warmUpAmplitude;
            break;
        }
            
        case EnvelopePhase::ATTACK: {
            // Calculate normalized time within attack phase
            float attackSamples = attack * sampleRate;
            float t = (attackSamples > 0.0f) 
                ? (phaseTime / attackSamples) 
                : 1.0f;
            t = std::clamp(t, 0.0f, 1.0f);
            
            // Apply attack curve
            float curved = applyCurve(t, attackCurve);
            
            // Attack goes from 0 to 1.0 (peak)
            currentValue = curved;
            break;
        }
            
        case EnvelopePhase::DECAY: {
            // Calculate normalized time within decay phase
            float decaySamples = decay * sampleRate;
            float t = (decaySamples > 0.0f) 
                ? (phaseTime / decaySamples) 
                : 1.0f;
            t = std::clamp(t, 0.0f, 1.0f);
            
            // Apply decay curve
            float curved = applyCurve(t, decayCurve);
            
            // Decay goes from 1.0 (peak) to sustain level
            currentValue = 1.0f - curved * (1.0f - sustain);
            break;
        }
            
        case EnvelopePhase::SUSTAIN:
            // Hold at sustain level
            currentValue = sustain;
            break;
            
        case EnvelopePhase::RELEASE: {
            // Calculate normalized time within release phase
            float releaseSamples = releaseTime * sampleRate;
            float t = (releaseSamples > 0.0f) 
                ? (phaseTime / releaseSamples) 
                : 1.0f;
            t = std::clamp(t, 0.0f, 1.0f);
            
            // Apply release curve
            float curved = applyCurve(t, releaseCurve);
            
            // Release goes from current value to 0
            // We need to store the value at the start of release
            // For now, we'll use sustain as the starting point
            currentValue = sustain * (1.0f - curved);
            break;
        }
    }
}

void DualPhaseEnvelope::transitionToNextPhase() {
    switch (currentPhase) {
        case EnvelopePhase::IDLE:
            // No transition from idle (must be triggered)
            break;
            
        case EnvelopePhase::WARMUP: {
            // Transition to ATTACK after warm-up duration
            float warmUpDurationSamples = warmUpDuration * sampleRate;
            // Keep the final warm-up sample at the configured amplitude. The
            // following sample starts the attack at zero.
            if (phaseTime > warmUpDurationSamples) {
                currentPhase = EnvelopePhase::ATTACK;
                phaseTime = 0.0f;
                // Ensure phase continuity: start attack from 0
                currentValue = 0.0f;
            }
            break;
        }
            
        case EnvelopePhase::ATTACK: {
            // Transition to DECAY after attack time
            float attackSamples = attack * sampleRate;
            if (phaseTime >= attackSamples) {
                currentPhase = EnvelopePhase::DECAY;
                phaseTime = 0.0f;
                // currentValue should be at peak (1.0)
                currentValue = 1.0f;
            }
            break;
        }
            
        case EnvelopePhase::DECAY: {
            // Transition to SUSTAIN after decay time
            float decaySamples = decay * sampleRate;
            if (phaseTime >= decaySamples) {
                currentPhase = EnvelopePhase::SUSTAIN;
                phaseTime = 0.0f;
                currentValue = sustain;
            }
            break;
        }
            
        case EnvelopePhase::SUSTAIN:
            // Sustain indefinitely until release() is called
            // For kick drums, sustain is typically 0, so we can auto-release
            if (sustain <= 0.0f) {
                currentPhase = EnvelopePhase::RELEASE;
                phaseTime = 0.0f;
            }
            break;
            
        case EnvelopePhase::RELEASE: {
            // Transition to IDLE after release time
            float releaseSamples = releaseTime * sampleRate;
            if (phaseTime >= releaseSamples) {
                currentPhase = EnvelopePhase::IDLE;
                phaseTime = 0.0f;
                currentValue = 0.0f;
            }
            break;
        }
    }
}

} // namespace KickDrum
