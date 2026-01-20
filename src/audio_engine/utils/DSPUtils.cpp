#include "DSPUtils.h"
#include <cmath>
#include <algorithm>

namespace KickDrum {
namespace DSPUtils {

float softClip(float input) {
    // Cubic soft clipping algorithm
    // This provides smooth limiting without harsh distortion
    
    const float absInput = std::abs(input);
    
    // For small signals, pass through unchanged
    if (absInput < 2.0f / 3.0f) {
        return input;
    }
    
    // For signals approaching ±1.0, apply smooth compression
    if (absInput < 1.0f) {
        const float sign = (input >= 0.0f) ? 1.0f : -1.0f;
        const float temp = 2.0f - 3.0f * absInput;
        return sign * (3.0f - temp * temp) / 3.0f;
    }
    
    // For signals exceeding ±1.0, hard limit to ±1.0
    return (input >= 0.0f) ? 1.0f : -1.0f;
}

void softClipBuffer(float* buffer, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        buffer[i] = softClip(buffer[i]);
    }
}

bool isBufferValid(const float* buffer, size_t numSamples, size_t* outInvalidIndex) {
    for (size_t i = 0; i < numSamples; ++i) {
        if (!isValid(buffer[i])) {
            if (outInvalidIndex != nullptr) {
                *outInvalidIndex = i;
            }
            return false;
        }
    }
    return true;
}

size_t sanitizeBuffer(float* buffer, size_t numSamples) {
    size_t invalidCount = 0;
    
    for (size_t i = 0; i < numSamples; ++i) {
        if (!isValid(buffer[i])) {
            buffer[i] = 0.0f;
            invalidCount++;
        }
    }
    
    return invalidCount;
}

void applyMasterLevel(float* buffer, size_t numSamples, float level) {
    // Clamp level to valid range [0.0, 1.0]
    level = clamp(level, 0.0f, 1.0f);
    
    for (size_t i = 0; i < numSamples; ++i) {
        buffer[i] *= level;
    }
}

} // namespace DSPUtils
} // namespace KickDrum
