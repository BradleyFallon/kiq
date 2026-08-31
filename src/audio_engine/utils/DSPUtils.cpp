#include "DSPUtils.h"
#include <cmath>
#include <algorithm>

namespace KickDrum {
namespace DSPUtils {

float softClip(float input) {
    // Keep the body untouched below the knee, then approach full scale with a
    // continuous, monotonic tanh shoulder. The old cubic branch jumped at the
    // knee and then decreased as its input increased, which created an audible
    // crackle on otherwise clean kick hits.
    constexpr float knee = 2.0f / 3.0f;
    const float absInput = std::abs(input);

    if (absInput <= knee) {
        return input;
    }

    constexpr float shoulder = 1.0f - knee;
    const float distanceIntoShoulder = (absInput - knee) / shoulder;
    const float magnitude = knee + shoulder * std::tanh(distanceIntoShoulder);
    return std::copysign(magnitude, input);
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
