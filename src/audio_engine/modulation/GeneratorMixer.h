#pragma once

namespace KickDrum {

/**
 * @brief Mixer for combining sine driver, modulated harmonics, and modulated noise
 * 
 * The GeneratorMixer combines the three synthesis sources with independent level
 * controls for each:
 *   - Sine Driver: The main tone and transient source
 *   - Modulated Harmonics: Ring-modulated harmonic content
 *   - Modulated Noise: Ring-modulated noise texture
 * 
 * The mixer performs a weighted sum:
 *   output = sine × sineLevel + harmonics × harmonicLevel + noise × noiseLevel
 * 
 * All level parameters are in the range [0.0, 1.0] where:
 *   - 0.0 = silent (0%)
 *   - 1.0 = full level (100%)
 * 
 * This implements Requirements 1.7, 4.2, 4.3, 4.4:
 *   - 1.7: Mix direct sine, ring-modulated harmonics, and ring-modulated noise
 *   - 4.2: Sine driver level parameter (0% to 100%)
 *   - 4.3: Harmonic level parameter (0% to 100%)
 *   - 4.4: Noise level parameter (0% to 100%)
 */
class GeneratorMixer {
public:
    /**
     * @brief Construct a new Generator Mixer with default levels (all 0.0)
     */
    GeneratorMixer();

    /**
     * @brief Set the sine driver level
     * @param level Level in range [0.0, 1.0] (0% to 100%)
     */
    void setSineLevel(float level);

    /**
     * @brief Get the current sine driver level
     * @return Current sine level in range [0.0, 1.0]
     */
    float getSineLevel() const;

    /**
     * @brief Set the harmonic level
     * @param level Level in range [0.0, 1.0] (0% to 100%)
     */
    void setHarmonicLevel(float level);

    /**
     * @brief Get the current harmonic level
     * @return Current harmonic level in range [0.0, 1.0]
     */
    float getHarmonicLevel() const;

    /**
     * @brief Set the noise level
     * @param level Level in range [0.0, 1.0] (0% to 100%)
     */
    void setNoiseLevel(float level);

    /**
     * @brief Get the current noise level
     * @return Current noise level in range [0.0, 1.0]
     */
    float getNoiseLevel() const;

    /**
     * @brief Mix the three generator sources
     * @param sine Sine driver sample
     * @param modulatedHarmonic Ring-modulated harmonic sample
     * @param modulatedNoise Ring-modulated noise sample
     * @return Mixed output sample
     * 
     * The mixing formula is:
     *   output = sine × sineLevel + modulatedHarmonic × harmonicLevel + modulatedNoise × noiseLevel
     */
    float mix(float sine, float modulatedHarmonic, float modulatedNoise);

private:
    float sineLevel_;      ///< Sine driver level (0.0 to 1.0)
    float harmonicLevel_;  ///< Harmonic level (0.0 to 1.0)
    float noiseLevel_;     ///< Noise level (0.0 to 1.0)
};

} // namespace KickDrum
