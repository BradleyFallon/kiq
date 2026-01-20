#pragma once

#include <cmath>

namespace KickDrum {

/**
 * @brief Harmonic oscillator adding character, tuning, and tonal color
 * 
 * The HarmonicMembrane generates harmonic content at a frequency ratio
 * relative to the Sine Driver's base frequency. It provides additional
 * tonal character and serves as a modulation source for ring modulation.
 * 
 * The frequency is calculated as: frequency = baseFrequency × ratio
 * where ratio ranges from 0.5x to 8.0x.
 * 
 * Implementation uses phase accumulator (0.0 to 1.0) for precise frequency
 * control and maintains phase continuity during ratio changes.
 */
class HarmonicMembrane {
public:
    /**
     * @brief Construct a new Harmonic Membrane
     */
    HarmonicMembrane();

    /**
     * @brief Initialize the oscillator with a sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     */
    void initialize(float sampleRate);

    /**
     * @brief Set the base frequency (from Sine Driver)
     * @param frequency Base frequency in Hz
     */
    void setBaseFrequency(float frequency);

    /**
     * @brief Get the current base frequency
     * @return Current base frequency in Hz
     */
    float getBaseFrequency() const;

    /**
     * @brief Set the frequency ratio
     * @param ratio Frequency ratio (0.5x to 8.0x)
     */
    void setRatio(float ratio);

    /**
     * @brief Get the current frequency ratio
     * @return Current frequency ratio
     */
    float getRatio() const;

    /**
     * @brief Get the actual output frequency
     * @return Actual frequency in Hz (baseFrequency × ratio)
     */
    float getFrequency() const;

    /**
     * @brief Reset the phase to 0
     * 
     * This is useful for ensuring consistent phase at the start of a note
     * or when synchronizing multiple oscillators.
     */
    void reset();

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
    float baseFrequency_;   ///< Base frequency from Sine Driver in Hz
    float ratio_;           ///< Frequency ratio (0.5x to 8.0x)
    float phase_;           ///< Current phase (0.0 to 1.0)
    float phaseIncrement_;  ///< Phase increment per sample
    bool initialized_;      ///< Initialization flag

    /**
     * @brief Update the phase increment based on current frequency
     */
    void updatePhaseIncrement();
};

} // namespace KickDrum
