#pragma once

#include <cmath>

namespace KickDrum {

/**
 * @brief Sine wave oscillator serving as the kick body
 * 
 * The SineDriver generates a pure sine wave at a specified frequency using
 * a phase accumulator for sample-accurate frequency control.
 * 
 * Implementation uses phase accumulator (0.0 to 1.0) for precise frequency
 * control and ensures phase continuity during frequency changes.
 */
class SineDriver {
public:
    /**
     * @brief Construct a new Sine Driver
     */
    SineDriver();

    /**
     * @brief Initialize the oscillator with a sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     */
    void initialize(float sampleRate);

    /**
     * @brief Set the oscillator frequency
     * @param frequency Frequency in Hz
     */
    void setFrequency(float frequency);

    /**
     * @brief Get the current frequency
     * @return Current frequency in Hz
     */
    float getFrequency() const;

    /**
     * @brief Reset the phase to 0
     * 
     * This is useful for ensuring consistent phase at the start of a note
     * or when synchronizing multiple oscillators.
     */
    void reset();

    /** Set phase in normalized cycles. Values wrap into [0, 1). */
    void setPhase(float phase);

    /** Get the current normalized phase. */
    float getPhase() const;

    /**
     * @brief Generate the next sample
     * @return Sine wave sample in range [-1.0, 1.0]
     */
    float generate();

    /**
     * @brief Check if the oscillator is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

private:
    float sampleRate_;      ///< Sample rate in Hz
    float frequency_;       ///< Current frequency in Hz
    float phase_;           ///< Current phase (0.0 to 1.0)
    float phaseIncrement_;  ///< Phase increment per sample
    bool initialized_;      ///< Initialization flag

    /**
     * @brief Update the phase increment based on current frequency
     */
    void updatePhaseIncrement();
};

} // namespace KickDrum
