#pragma once

#include "DualPhaseEnvelope.h"

namespace KickDrum {

/**
 * @brief Pitch envelope for frequency modulation
 * 
 * This class wraps a DualPhaseEnvelope and applies a depth parameter
 * to convert the envelope output (0.0 to 1.0) into a frequency offset in Hz.
 * 
 * The pitch envelope is typically used to create the characteristic pitch sweep
 * of a kick drum, where the pitch starts high and quickly decays to the base pitch.
 * 
 * Requirements validated:
 * - 2.7: Pitch envelope depth adjustment (0Hz to 2000Hz range)
 */
class PitchEnvelope {
public:
    /**
     * @brief Construct a new Pitch Envelope
     * 
     * @param sampleRate Audio sample rate in Hz
     */
    explicit PitchEnvelope(float sampleRate);
    
    /**
     * @brief Destructor
     */
    ~PitchEnvelope();
    
    // ========================================================================
    // Pitch Envelope Parameters
    // ========================================================================
    
    /**
     * @brief Set pitch envelope depth
     * 
     * The depth parameter controls how much the envelope affects the pitch.
     * A depth of 0Hz means no pitch modulation.
     * A depth of 2000Hz means the pitch can sweep up to 2000Hz above the base pitch.
     * 
     * @param depth Depth in Hz (0Hz to 2000Hz)
     */
    void setDepth(float depth);
    
    /**
     * @brief Get pitch envelope depth
     * 
     * @return Depth in Hz
     */
    float getDepth() const { return depth; }
    
    // ========================================================================
    // Envelope Control
    // ========================================================================
    
    /**
     * @brief Trigger the pitch envelope from the beginning
     */
    void trigger();
    
    /**
     * @brief Release the pitch envelope
     */
    void release();
    
    /**
     * @brief Reset the pitch envelope to idle state
     */
    void reset();
    
    /**
     * @brief Advance the pitch envelope by one sample
     * 
     * Call this once per audio sample to update the envelope state.
     */
    void advance();
    
    /**
     * @brief Get the current pitch offset in Hz
     * 
     * This returns the frequency offset that should be added to the base pitch.
     * The value ranges from 0Hz (at the end of the envelope) to the depth value
     * (at the peak of the envelope).
     * 
     * @return Frequency offset in Hz
     */
    float getValue() const;
    
    /**
     * @brief Check if the pitch envelope is currently active
     * 
     * @return true if envelope is not in IDLE phase
     */
    bool isActive() const;
    
    /**
     * @brief Set the sample rate
     * 
     * Call this when the audio engine sample rate changes.
     * 
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(float sampleRate);
    
    // ========================================================================
    // Access to Underlying Envelope
    // ========================================================================
    
    /**
     * @brief Get access to the underlying dual-phase envelope
     * 
     * This allows direct configuration of the envelope parameters
     * (warm-up, attack, decay, sustain, release, curves).
     * 
     * @return Reference to the underlying DualPhaseEnvelope
     */
    DualPhaseEnvelope& getEnvelope() { return *envelope; }
    
    /**
     * @brief Get const access to the underlying dual-phase envelope
     * 
     * @return Const reference to the underlying DualPhaseEnvelope
     */
    const DualPhaseEnvelope& getEnvelope() const { return *envelope; }
    
private:
    DualPhaseEnvelope* envelope;  // Underlying envelope
    float depth;                   // Pitch modulation depth in Hz (0 to 2000)
};

} // namespace KickDrum
