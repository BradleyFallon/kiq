#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace KickDrum {

class VoiceAllocator;
class ParameterManager;
class ParameterEventQueue;

/**
 * @brief Main audio engine coordinating trajectory-driven kick synthesis
 * 
 * The AudioEngine integrates one-shot voice allocation and parameter
 * management to generate the final audio output.
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

    /** Allocate the render buffer before realtime processing starts. */
    void prepare(std::size_t maxSamplesPerBlock);

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
     * @brief Queue a note trigger for the start of the next audio block
     *
     * This is the thread-safe entry point for UI and other non-audio threads.
     * If several triggers arrive before the next block, the most recent one is
     * used.
     */
    void enqueueNoteOn(int note, float velocity);

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

    /** Set the authoritative output gain in the [0, 1] range. */
    void setOutputGain(float gain);

    /** Get the current output gain. */
    float getOutputGain() const;

    /** Maximum output peak since the previous read, in the [0, 1] range. */
    float getOutputPeak() const;

    /** Whether the signal reached full scale since the previous read. */
    bool getOutputClip() const;

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
     * This is a convenience method that updates KickParams, active voices,
     * and the ParameterManager immediately.
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
