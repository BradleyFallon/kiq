#pragma once

#include "EnvelopeCurves.h"

namespace KickDrum {

/**
 * @brief Envelope phase states
 * 
 * The dual-phase envelope progresses through these phases:
 * IDLE → WARMUP → ATTACK → DECAY → SUSTAIN → RELEASE → IDLE
 */
enum class EnvelopePhase {
    IDLE,       // Envelope is not active
    WARMUP,     // Pre-transient phase building speaker momentum
    ATTACK,     // Initial transient attack
    DECAY,      // Decay from peak to sustain level
    SUSTAIN,    // Sustained level (typically 0 for kick drums)
    RELEASE     // Final release to silence
};

/**
 * @brief Dual-phase envelope system for kick drum synthesis
 * 
 * This envelope system provides two distinct phases:
 * 1. Warm-Up Phase: Pre-transient sweep that builds speaker momentum
 * 2. Transient/Decay Phase: Traditional ADSR envelope for the main kick sound
 * 
 * The warm-up phase sweeps from a low start frequency to the base pitch,
 * creating a subtle pre-transient that adds weight and punch to the kick drum.
 * 
 * Requirements validated:
 * - 2.1: Warm-Up Phase envelope controlling pre-transient sweep
 * - 2.2: Transient/Decay Phase envelope controlling main kick drum
 * - 2.3: Warm-up duration adjustment (0ms to 100ms)
 * - 2.4: Warm-up frequency sweep range (5Hz to 50Hz start frequency)
 * - 2.5: Warm-up amplitude level adjustment
 * - 2.6: ADSR times adjustment
 * - 2.8: Curve shape selection
 * - 2.9: Independent curve shaping per segment
 * - 2.11: Phase continuity between warm-up and transient/decay
 */
class DualPhaseEnvelope {
public:
    /**
     * @brief Construct a new Dual Phase Envelope
     * 
     * @param sampleRate Audio sample rate in Hz
     */
    explicit DualPhaseEnvelope(float sampleRate);
    
    // ========================================================================
    // Warm-Up Phase Parameters
    // ========================================================================
    
    /**
     * @brief Set warm-up phase duration
     * 
     * @param duration Duration in seconds (0.0 to 0.1 for 0ms to 100ms)
     */
    void setWarmUpDuration(float duration);
    
    /**
     * @brief Set warm-up phase start frequency
     * 
     * This is the frequency at the beginning of the warm-up sweep.
     * The frequency will sweep from this value to the base pitch.
     * 
     * @param frequency Start frequency in Hz (5Hz to 50Hz)
     */
    void setWarmUpStartFrequency(float frequency);
    
    /**
     * @brief Set warm-up phase amplitude
     * 
     * @param amplitude Amplitude level (0.0 to 1.0)
     */
    void setWarmUpAmplitude(float amplitude);
    
    /**
     * @brief Get warm-up phase duration
     * 
     * @return Duration in seconds
     */
    float getWarmUpDuration() const { return warmUpDuration; }
    
    /**
     * @brief Get warm-up phase start frequency
     * 
     * @return Start frequency in Hz
     */
    float getWarmUpStartFrequency() const { return warmUpStartFreq; }
    
    /**
     * @brief Get warm-up phase amplitude
     * 
     * @return Amplitude level (0.0 to 1.0)
     */
    float getWarmUpAmplitude() const { return warmUpAmplitude; }
    
    // ========================================================================
    // Transient/Decay Phase Parameters (ADSR)
    // ========================================================================
    
    /**
     * @brief Set attack time
     * 
     * @param time Attack time in seconds
     */
    void setAttack(float time);
    
    /**
     * @brief Set decay time
     * 
     * @param time Decay time in seconds
     */
    void setDecay(float time);
    
    /**
     * @brief Set sustain level
     * 
     * @param level Sustain level (0.0 to 1.0)
     */
    void setSustain(float level);
    
    /**
     * @brief Set release time
     * 
     * @param time Release time in seconds
     */
    void setRelease(float time);
    
    /**
     * @brief Get attack time
     * 
     * @return Attack time in seconds
     */
    float getAttack() const { return attack; }
    
    /**
     * @brief Get decay time
     * 
     * @return Decay time in seconds
     */
    float getDecay() const { return decay; }
    
    /**
     * @brief Get sustain level
     * 
     * @return Sustain level (0.0 to 1.0)
     */
    float getSustain() const { return sustain; }
    
    /**
     * @brief Get release time
     * 
     * @return Release time in seconds
     */
    float getRelease() const { return releaseTime; }
    
    // ========================================================================
    // Curve Shaping
    // ========================================================================
    
    /**
     * @brief Set attack curve type
     * 
     * @param curve Curve type for attack phase
     */
    void setAttackCurve(CurveType curve);
    
    /**
     * @brief Set decay curve type
     * 
     * @param curve Curve type for decay phase
     */
    void setDecayCurve(CurveType curve);
    
    /**
     * @brief Set release curve type
     * 
     * @param curve Curve type for release phase
     */
    void setReleaseCurve(CurveType curve);
    
    /**
     * @brief Get attack curve type
     * 
     * @return Curve type for attack phase
     */
    CurveType getAttackCurve() const { return attackCurve; }
    
    /**
     * @brief Get decay curve type
     * 
     * @return Curve type for decay phase
     */
    CurveType getDecayCurve() const { return decayCurve; }
    
    /**
     * @brief Get release curve type
     * 
     * @return Curve type for release phase
     */
    CurveType getReleaseCurve() const { return releaseCurve; }
    
    // ========================================================================
    // Envelope Control
    // ========================================================================
    
    /**
     * @brief Trigger the envelope from the beginning
     * 
     * Starts the envelope from IDLE, entering either WARMUP (if duration > 0)
     * or ATTACK (if warm-up duration is 0).
     */
    void trigger();
    
    /**
     * @brief Release the envelope
     * 
     * Transitions from current phase to RELEASE phase.
     */
    void release();
    
    /**
     * @brief Reset the envelope to idle state
     */
    void reset();
    
    /**
     * @brief Advance the envelope by one sample
     * 
     * Call this once per audio sample to update the envelope state.
     */
    void advance();
    
    /**
     * @brief Get the current envelope amplitude value
     * 
     * @return Current amplitude (0.0 to 1.0)
     */
    float getValue() const;
    
    /**
     * @brief Check if the envelope is currently active
     * 
     * @return true if envelope is not in IDLE phase
     */
    bool isActive() const;
    
    /**
     * @brief Get the current envelope phase
     * 
     * @return Current phase
     */
    EnvelopePhase getCurrentPhase() const { return currentPhase; }
    
    /**
     * @brief Set the sample rate
     * 
     * Call this when the audio engine sample rate changes.
     * 
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(float sampleRate);
    
private:
    // Warm-Up Phase parameters
    float warmUpDuration;      // Duration in seconds (0.0 to 0.1)
    float warmUpStartFreq;     // Start frequency in Hz (5 to 50)
    float warmUpAmplitude;     // Amplitude level (0.0 to 1.0)
    
    // Transient/Decay Phase parameters (ADSR)
    float attack;              // Attack time in seconds
    float decay;               // Decay time in seconds
    float sustain;             // Sustain level (0.0 to 1.0)
    float releaseTime;         // Release time in seconds
    
    // Curve shaping
    CurveType attackCurve;
    CurveType decayCurve;
    CurveType releaseCurve;
    
    // State
    EnvelopePhase currentPhase;
    float phaseTime;           // Time within current phase (in samples)
    float sampleRate;
    
    // Cached values for efficiency
    float currentValue;        // Current envelope output value
    
    // Helper methods
    void updateValue();        // Calculate current envelope value based on phase
    void transitionToNextPhase(); // Handle phase transitions
};

} // namespace KickDrum
