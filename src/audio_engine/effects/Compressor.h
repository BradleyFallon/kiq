#pragma once

#include <cmath>

namespace KickDrum {

/**
 * @brief Dynamic range compressor with ballistics
 * 
 * The Compressor reduces the dynamic range of audio signals by applying
 * gain reduction to signals that exceed a threshold. It uses attack and
 * release times (ballistics) to smooth the gain reduction envelope,
 * preventing abrupt changes that would cause distortion.
 * 
 * Compression algorithm:
 * 1. Convert input to dB: inputDb = 20 × log10(abs(input))
 * 2. Calculate gain reduction:
 *    if inputDb > threshold:
 *      gainReductionDb = (inputDb - threshold) × (1 - 1/ratio)
 *    else:
 *      gainReductionDb = 0
 * 3. Smooth gain reduction with attack/release envelope
 * 4. Apply gain reduction: compressed = input × 10^(-gainReduction/20)
 * 5. Mix dry and wet: output = input × (1 - mix) + compressed × mix
 * 
 * Parameters:
 * - Threshold: Level above which compression is applied (in dB)
 * - Ratio: Amount of compression (1.0 = no compression, 20.0 = heavy compression)
 * - Attack: Time to reach full compression (in seconds)
 * - Release: Time to return to no compression (in seconds)
 * - Mix: Dry/wet blend (0.0 = fully dry, 1.0 = fully compressed)
 */
class Compressor {
public:
    /**
     * @brief Construct a new Compressor with default parameters
     */
    Compressor();

    /**
     * @brief Initialize the compressor with a sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     */
    void initialize(float sampleRate);

    /**
     * @brief Set the compression threshold
     * @param thresholdDb Threshold in dB (typically -60dB to 0dB)
     *                    Signals above this level will be compressed
     */
    void setThreshold(float thresholdDb);

    /**
     * @brief Get the current threshold
     * @return Threshold in dB
     */
    float getThreshold() const;

    /**
     * @brief Set the compression ratio
     * @param ratio Compression ratio (1.0 to 20.0)
     *              1.0 = no compression (unity gain)
     *              2.0 = 2:1 compression
     *              20.0 = 20:1 compression (heavy limiting)
     */
    void setRatio(float ratio);

    /**
     * @brief Get the current compression ratio
     * @return Compression ratio
     */
    float getRatio() const;

    /**
     * @brief Set the attack time
     * @param attackSeconds Attack time in seconds (typically 0.0001 to 0.1)
     *                      Time for compressor to reach full gain reduction
     */
    void setAttack(float attackSeconds);

    /**
     * @brief Get the current attack time
     * @return Attack time in seconds
     */
    float getAttack() const;

    /**
     * @brief Set the release time
     * @param releaseSeconds Release time in seconds (typically 0.01 to 1.0)
     *                       Time for compressor to return to no gain reduction
     */
    void setRelease(float releaseSeconds);

    /**
     * @brief Get the current release time
     * @return Release time in seconds
     */
    float getRelease() const;

    /**
     * @brief Set the dry/wet mix
     * @param mix Mix amount in range [0.0, 1.0]
     *            0.0 = fully dry (no compression)
     *            1.0 = fully wet (full compression)
     */
    void setMix(float mix);

    /**
     * @brief Get the current mix amount
     * @return Mix value in range [0.0, 1.0]
     */
    float getMix() const;

    /**
     * @brief Process a single audio sample through the compressor
     * @param input Input sample
     * @return Compressed output sample
     */
    float process(float input);

    /**
     * @brief Reset the compressor state
     * 
     * Resets the gain reduction envelope to 0, useful when starting
     * a new note or clearing the compressor state.
     */
    void reset();

    /**
     * @brief Check if the compressor is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

    /**
     * @brief Get the current gain reduction amount
     * @return Current gain reduction in dB (always >= 0)
     */
    float getGainReduction() const;

private:
    float sampleRate_;          ///< Sample rate in Hz
    float thresholdDb_;         ///< Threshold in dB
    float ratio_;               ///< Compression ratio
    float attackSeconds_;       ///< Attack time in seconds
    float releaseSeconds_;      ///< Release time in seconds
    float mix_;                 ///< Dry/wet mix (0.0 to 1.0)
    
    float envelopeDb_;          ///< Current gain reduction envelope in dB
    float attackCoeff_;         ///< Attack coefficient for envelope smoothing
    float releaseCoeff_;        ///< Release coefficient for envelope smoothing
    
    bool initialized_;          ///< Initialization flag

    /**
     * @brief Update attack and release coefficients based on times and sample rate
     */
    void updateCoefficients();

    /**
     * @brief Convert linear amplitude to dB
     * @param linear Linear amplitude value
     * @return Value in dB (or -infinity for zero input)
     */
    static float linearToDb(float linear);

    /**
     * @brief Convert dB to linear amplitude
     * @param db Value in dB
     * @return Linear amplitude value
     */
    static float dbToLinear(float db);
};

} // namespace KickDrum
