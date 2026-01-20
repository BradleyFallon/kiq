#pragma once

#include <cmath>
#include <cstddef>

namespace KickDrum {
namespace DSPUtils {

/**
 * @brief Apply soft clipping to prevent hard clipping at ±1.0
 * 
 * Uses a cubic soft clipping algorithm that smoothly limits the signal
 * to the range [-1.0, 1.0] without harsh distortion.
 * 
 * @param input Input sample value
 * @return Soft-clipped output value in range [-1.0, 1.0]
 * 
 * Algorithm:
 * - For |x| < 2/3: output = x
 * - For 2/3 <= |x| < 1: output = (3 - (2-3x)^2) / 3 * sign(x)
 * - For |x| >= 1: output = sign(x)
 */
float softClip(float input);

/**
 * @brief Apply soft clipping to an audio buffer in-place
 * 
 * @param buffer Audio buffer to process
 * @param numSamples Number of samples in the buffer
 */
void softClipBuffer(float* buffer, size_t numSamples);

/**
 * @brief Check if a value is valid (not NaN or infinity)
 * 
 * @param value Value to check
 * @return true if value is finite, false if NaN or infinity
 */
inline bool isValid(float value) {
    return std::isfinite(value);
}

/**
 * @brief Check if an audio buffer contains any invalid values (NaN or infinity)
 * 
 * @param buffer Audio buffer to check
 * @param numSamples Number of samples in the buffer
 * @param outInvalidIndex Optional output parameter to receive the index of the first invalid value
 * @return true if all values are valid, false if any NaN or infinity found
 */
bool isBufferValid(const float* buffer, size_t numSamples, size_t* outInvalidIndex = nullptr);

/**
 * @brief Replace invalid values (NaN or infinity) with zero
 * 
 * @param buffer Audio buffer to sanitize
 * @param numSamples Number of samples in the buffer
 * @return Number of invalid values that were replaced
 */
size_t sanitizeBuffer(float* buffer, size_t numSamples);

/**
 * @brief Apply master level scaling to an audio buffer
 * 
 * @param buffer Audio buffer to scale
 * @param numSamples Number of samples in the buffer
 * @param level Master level (0.0 to 1.0)
 */
void applyMasterLevel(float* buffer, size_t numSamples, float level);

/**
 * @brief Prevent denormal numbers by adding a small DC offset
 * 
 * Denormal numbers can cause significant CPU overhead. This function
 * adds a tiny DC offset to prevent denormals in feedback paths.
 * 
 * @param value Input value
 * @return Value with denormal prevention applied
 */
inline float preventDenormal(float value) {
    constexpr float DC_OFFSET = 1e-25f;
    return value + DC_OFFSET;
}

/**
 * @brief Clamp a value to a specified range
 * 
 * @param value Value to clamp
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief Convert MIDI note number to frequency in Hz
 * 
 * Uses the standard MIDI tuning formula:
 * frequency = 440 * 2^((note - 69) / 12)
 * 
 * Where MIDI note 69 = A4 = 440 Hz
 * 
 * @param note MIDI note number (0-127)
 * @return Frequency in Hz
 * 
 * Examples:
 * - MIDI note 60 (C4) = 261.63 Hz
 * - MIDI note 69 (A4) = 440.00 Hz
 * - MIDI note 36 (C2) = 65.41 Hz (typical kick drum range)
 */
inline float midiNoteToFrequency(int note) {
    // Standard MIDI tuning: A4 (note 69) = 440 Hz
    // Formula: f = 440 * 2^((note - 69) / 12)
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

} // namespace DSPUtils
} // namespace KickDrum
