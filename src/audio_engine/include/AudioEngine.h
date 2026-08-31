#pragma once

#include "SampleLayerData.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace KickDrum {

class VoiceAllocator;
class ParameterManager;
class ParameterEventQueue;
struct KickParams;
enum class KickParameterId : std::uint32_t;

/**
 * @brief Main audio engine coordinating trajectory-driven kick synthesis
 * 
 * The AudioEngine integrates one-shot voice allocation and parameter
 * management to generate the final audio output.
 */
class AudioEngine {
public:
    static constexpr std::size_t kMaxRealtimeParameterEvents = 2048;
    static constexpr std::size_t kMaxRealtimeNoteEvents = 1024;

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
     * @brief Trigger a note from the audio thread or while processing is stopped
     * @param note MIDI note number (0-127)
     * @param velocity MIDI velocity (0.0-1.0)
     *
     * Live UI/control threads must use enqueueNoteOn().
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
     * Schedule a typed parameter change in the next processBlock call.
     *
     * Audio-thread only. Storage is fixed and this call never allocates or
     * locks. Returns false when the fixed block queue is full or the ID is
     * invalid. Offsets at or beyond the block end establish next-block state.
     */
    bool scheduleParameterEvent(KickParameterId id, float value,
                                std::uint32_t sampleOffset) noexcept;

    /** Audio-thread-only, fixed-storage note event scheduling. */
    bool scheduleNoteOnEvent(int note, float velocity,
                             std::uint32_t sampleOffset) noexcept;
    bool scheduleNoteOffEvent(int note, std::uint32_t sampleOffset) noexcept;

    /** Discard typed events not yet consumed by processBlock. */
    void clearScheduledEvents() noexcept;

    /**
     * Apply queued UI and typed events as a zero-length block boundary.
     * Used by hosts when flushing parameters without an audio buffer.
     */
    void flushScheduledEvents();

    /**
     * @brief Enable or disable sample-accurate repeating UI audition hits
     * @param enabled Whether audition looping is active
     * @param bpm Tempo in beats per minute (clamped to 40-240)
     *
     * This method is safe to call from the UI thread. Enabling the loop
     * schedules its first hit at the start of the next processed block.
     */
    void setAuditionLoop(bool enabled, float bpm);

    /**
     * Install immutable mono audio for the optional sample layer.
     *
     * The source is copied and finite-sanitized on the calling thread, then
     * adopted at the next audio callback. Active hits retain their snapshot.
     */
    void setSampleLayer(std::shared_ptr<const SampleLayerData> sampleLayer);

    /**
     * Stage a complete parameter/sample restore for one audio-block boundary.
     * Safe from a control thread while processing is active.
     */
    std::uint64_t setStateSnapshot(
        const KickParams& params,
        std::shared_ptr<const SampleLayerData> sampleLayer);

    /** Revision of the complete state snapshot crossed by the audio thread. */
    std::uint64_t getAppliedStateRevision() const noexcept;

    /** Return the currently requested immutable source, if any. */
    std::shared_ptr<const SampleLayerData> getSampleLayer() const;

    /** Disable the sample source for future hits. */
    void clearSampleLayer();

    /** Audio-thread/currently-inactive snapshot of authoritative parameters. */
    KickParams getParams() const;

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
     * Use this producer-safe queue for UI/control-thread changes. Code already
     * running on the audio thread should use scheduleParameterEvent() to avoid
     * string construction and synchronization.
     */
    ParameterEventQueue* getParameterEventQueue();

    /**
     * @brief Set a parameter value immediately (non-sample-accurate)
     * 
     * This is a convenience method that updates KickParams, active voices,
     * and the ParameterManager immediately.
     * 
     * UI/control threads can use getParameterEventQueue() for timed changes.
     * Code already on the audio thread should use scheduleParameterEvent().
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
