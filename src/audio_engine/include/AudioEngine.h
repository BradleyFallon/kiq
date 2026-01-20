#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace KickDrum {

class VoiceAllocator;
class ParameterManager;
class EffectsChain;
class ParameterEventQueue;

/**
 * @brief Main audio engine coordinating synthesis, effects, and output
 * 
 * The AudioEngine integrates voice allocation, effects processing, and
 * parameter management to generate the final audio output.
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    /**
     * @brief Initialize the audio engine with the given sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     */
    void initialize(float sampleRate);

    /**
     * @brief Process an audio buffer
     * @param outputBuffer Output buffer to fill with audio samples
     * @param numSamples Number of samples to generate
     * @param numChannels Number of output channels (1 = mono, 2 = stereo)
     */
    void processBlock(float* outputBuffer, size_t numSamples, size_t numChannels);

    /**
     * @brief Trigger a note
     * @param note MIDI note number (0-127)
     * @param velocity MIDI velocity (0.0-1.0)
     */
    void noteOn(int note, float velocity);

    /**
     * @brief Release a note
     * @param note MIDI note number (0-127)
     */
    void noteOff(int note);

    /**
     * @brief Release all active notes
     */
    void allNotesOff();

    /**
     * @brief Get the parameter manager
     */
    ParameterManager* getParameterManager();

    /**
     * @brief Get the current sample rate
     */
    float getSampleRate() const;

    /**
     * @brief Set the master output level
     * @param level Master level (0.0 to 1.0)
     * 
     * The master level is applied after all effects processing.
     * Values outside [0.0, 1.0] are clamped.
     */
    void setMasterLevel(float level);

    /**
     * @brief Get the current master output level
     * @return Master level (0.0 to 1.0)
     */
    float getMasterLevel() const;

    /**
     * @brief Enable or disable soft clipping
     * @param enable true to enable soft clipping, false to disable
     * 
     * When enabled, signals exceeding ±1.0 are smoothly limited
     * to prevent hard clipping distortion.
     */
    void setSoftClippingEnabled(bool enable);

    /**
     * @brief Check if soft clipping is enabled
     * @return true if enabled, false if disabled
     */
    bool isSoftClippingEnabled() const;

    /**
     * @brief Enable or disable NaN/infinity detection
     * @param enable true to enable detection, false to disable
     * 
     * When enabled, the audio engine checks for invalid values
     * (NaN or infinity) and resets synthesis state if detected.
     */
    void setNaNDetectionEnabled(bool enable);

    /**
     * @brief Check if NaN/infinity detection is enabled
     * @return true if enabled, false if disabled
     */
    bool isNaNDetectionEnabled() const;

    /**
     * @brief Get the effects chain for parameter control
     * @return Pointer to effects chain (may be null if not initialized)
     */
    EffectsChain* getEffectsChain();

    /**
     * @brief Get the voice allocator for direct access
     * @return Pointer to voice allocator (may be null if not initialized)
     */
    VoiceAllocator* getVoiceAllocator();

    /**
     * @brief Get the parameter event queue for sample-accurate parameter updates
     * @return Pointer to parameter event queue (may be null if not initialized)
     * 
     * Use this to schedule parameter changes at specific sample positions
     * within audio buffers for sample-accurate automation.
     */
    ParameterEventQueue* getParameterEventQueue();

    /**
     * @brief Set a parameter value immediately (non-sample-accurate)
     * 
     * This is a convenience method that updates the parameter in the
     * ParameterManager and schedules an immediate event (offset 0) in
     * the parameter event queue.
     * 
     * For sample-accurate control, use getParameterEventQueue() directly
     * and add events with specific sample offsets.
     * 
     * @param parameterId Parameter ID
     * @param value New parameter value
     */
    void setParameter(const std::string& parameterId, float value);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace KickDrum
