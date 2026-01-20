#pragma once

#include <cstdint>

namespace KickDrum {

/**
 * @brief White noise generator for transient texture
 * 
 * The NoiseGenerator produces uniform white noise in the range [-1.0, 1.0]
 * using a high-quality PRNG (xorshift algorithm). It provides a modulation
 * source for ring modulation and fills out the transient texture of the
 * kick drum.
 * 
 * The generator supports seed control for reproducibility, which is useful
 * for testing and creating consistent sounds.
 */
class NoiseGenerator {
public:
    /**
     * @brief Construct a new Noise Generator with default seed
     */
    NoiseGenerator();

    /**
     * @brief Construct a new Noise Generator with specified seed
     * @param seed Random seed for reproducibility
     */
    explicit NoiseGenerator(uint64_t seed);

    /**
     * @brief Set the random seed
     * @param seed Random seed value
     */
    void setSeed(uint64_t seed);

    /**
     * @brief Get the current seed
     * @return Current seed value
     */
    uint64_t getSeed() const;

    /**
     * @brief Reset the generator to its initial seed state
     * 
     * This resets the internal state to the seed value, allowing
     * the same sequence of random numbers to be generated again.
     */
    void reset();

    /**
     * @brief Generate the next white noise sample
     * @return White noise sample in range [-1.0, 1.0]
     */
    float generate();

private:
    uint64_t seed_;         ///< Initial seed value
    uint64_t state_;        ///< Current PRNG state

    /**
     * @brief Xorshift64 PRNG algorithm
     * @return Next pseudo-random 64-bit integer
     */
    uint64_t xorshift64();
};

} // namespace KickDrum
