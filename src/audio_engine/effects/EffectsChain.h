#pragma once

#include "Compressor.h"
#include "Reverb.h"

namespace KickDrum {

/**
 * @brief Effects chain coordinator for master effects processing
 * 
 * The EffectsChain coordinates the Compressor and Reverb effects in series,
 * applying them to the mixed audio output. The chain processes audio in the
 * following order:
 * 
 *   Input → Compressor → Reverb → Output
 * 
 * Each effect can be independently bypassed, allowing flexible routing:
 * - Both active: Input → Compressor → Reverb → Output
 * - Compressor bypassed: Input → Reverb → Output
 * - Reverb bypassed: Input → Compressor → Output
 * - Both bypassed: Input → Output (pass-through)
 * 
 * Requirements:
 * - 5.1: Apply compressor to mixed output before final output
 * - 5.2: Apply reverb to mixed output after compression
 * - 5.8: Allow bypassing of the Compressor independently
 * - 5.9: Allow bypassing of the Reverb independently
 */
class EffectsChain {
public:
    /**
     * @brief Construct a new EffectsChain with default parameters
     */
    EffectsChain();

    /**
     * @brief Initialize the effects chain with a sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     */
    void initialize(float sampleRate);

    /**
     * @brief Get the compressor instance for parameter control
     * @return Reference to the compressor
     */
    Compressor& getCompressor();

    /**
     * @brief Get the compressor instance (const version)
     * @return Const reference to the compressor
     */
    const Compressor& getCompressor() const;

    /**
     * @brief Get the reverb instance for parameter control
     * @return Reference to the reverb
     */
    Reverb& getReverb();

    /**
     * @brief Get the reverb instance (const version)
     * @return Const reference to the reverb
     */
    const Reverb& getReverb() const;

    /**
     * @brief Set the compressor bypass state
     * @param bypassed true to bypass the compressor, false to enable it
     * 
     * When bypassed, the compressor is skipped in the processing chain
     * and the input signal passes through unchanged.
     */
    void setCompressorBypassed(bool bypassed);

    /**
     * @brief Check if the compressor is bypassed
     * @return true if bypassed, false if active
     */
    bool isCompressorBypassed() const;

    /**
     * @brief Set the reverb bypass state
     * @param bypassed true to bypass the reverb, false to enable it
     * 
     * When bypassed, the reverb is skipped in the processing chain
     * and the input signal passes through unchanged.
     */
    void setReverbBypassed(bool bypassed);

    /**
     * @brief Check if the reverb is bypassed
     * @return true if bypassed, false if active
     */
    bool isReverbBypassed() const;

    /**
     * @brief Process a single audio sample through the effects chain
     * @param input Input sample from the mixer
     * @return Processed output sample
     * 
     * Processing order:
     * 1. Apply compressor (if not bypassed)
     * 2. Apply reverb (if not bypassed)
     * 3. Return processed sample
     */
    float process(float input);

    /**
     * @brief Reset the effects chain state
     * 
     * Resets both the compressor and reverb to their initial states,
     * clearing any internal buffers and envelopes.
     */
    void reset();

    /**
     * @brief Check if the effects chain is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

private:
    Compressor compressor_;         ///< Compressor effect (first in chain)
    Reverb reverb_;                 ///< Reverb effect (second in chain)
    bool compressorBypassed_;       ///< Compressor bypass state
    bool reverbBypassed_;           ///< Reverb bypass state
    bool initialized_;              ///< Initialization flag
};

} // namespace KickDrum
