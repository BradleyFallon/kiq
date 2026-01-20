#include "NoiseGenerator.h"
#include <cstdint>

namespace KickDrum {

// Default seed value (arbitrary non-zero value)
constexpr uint64_t DEFAULT_SEED = 0x123456789ABCDEF0ULL;

// Constant for converting uint64_t to float in range [-1.0, 1.0]
// We use the full 64-bit range and map it to [-1.0, 1.0]
constexpr double UINT64_TO_FLOAT = 2.0 / static_cast<double>(UINT64_MAX);

NoiseGenerator::NoiseGenerator()
    : seed_(DEFAULT_SEED)
    , state_(DEFAULT_SEED)
{
}

NoiseGenerator::NoiseGenerator(uint64_t seed)
    : seed_(seed != 0 ? seed : DEFAULT_SEED)  // Ensure seed is never zero
    , state_(seed != 0 ? seed : DEFAULT_SEED)
{
}

void NoiseGenerator::setSeed(uint64_t seed) {
    // Ensure seed is never zero (xorshift requires non-zero state)
    seed_ = (seed != 0) ? seed : DEFAULT_SEED;
    state_ = seed_;
}

uint64_t NoiseGenerator::getSeed() const {
    return seed_;
}

void NoiseGenerator::reset() {
    state_ = seed_;
}

float NoiseGenerator::generate() {
    // Generate next random value using xorshift64
    uint64_t randomValue = xorshift64();
    
    // Convert to float in range [-1.0, 1.0]
    // Map [0, UINT64_MAX] to [-1.0, 1.0]
    double normalized = static_cast<double>(randomValue) * UINT64_TO_FLOAT - 1.0;
    
    return static_cast<float>(normalized);
}

uint64_t NoiseGenerator::xorshift64() {
    // Xorshift64 algorithm - high quality PRNG
    // This is a well-tested algorithm with good statistical properties
    // Period: 2^64 - 1
    
    uint64_t x = state_;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state_ = x;
    
    return x;
}

} // namespace KickDrum
