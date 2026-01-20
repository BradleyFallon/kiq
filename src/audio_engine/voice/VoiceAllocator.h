#pragma once

#include "Voice.h"
#include <vector>
#include <cstdint>

namespace KickDrum {

/**
 * @brief VoiceAllocator manages a pool of voices for polyphony
 * 
 * The VoiceAllocator class manages a pool of 8 voices and implements
 * voice allocation and stealing strategies:
 * 
 * Voice Allocation Strategy:
 * 1. Find an idle voice (not active)
 * 2. If no idle voice, steal the oldest voice
 * 3. Trigger the allocated voice with note and velocity
 * 
 * Voice Stealing:
 * - When all voices are active and a new note arrives
 * - Steal the voice with the highest age (oldest)
 * - This ensures smooth polyphony without abrupt cutoffs
 * 
 * Requirements validated:
 * - 12.4: Handle polyphony up to 8 simultaneous voices
 */
class VoiceAllocator {
public:
    /**
     * @brief Construct a new VoiceAllocator
     * 
     * Creates a pool of 8 voices for polyphony.
     */
    VoiceAllocator();
    
    /**
     * @brief Initialize the voice allocator with a sample rate
     * 
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     */
    void initialize(float sampleRate);
    
    // ========================================================================
    // Voice Allocation
    // ========================================================================
    
    /**
     * @brief Allocate a voice for a MIDI note
     * 
     * Allocation strategy:
     * 1. Find an idle voice (not active)
     * 2. If no idle voice, steal the oldest voice
     * 3. Trigger the allocated voice with note and velocity
     * 
     * @param note MIDI note number (0-127)
     * @param velocity MIDI velocity (0.0 to 1.0)
     * @return Pointer to the allocated voice
     */
    Voice* allocateVoice(int note, float velocity);
    
    /**
     * @brief Release a voice by MIDI note number
     * 
     * Finds the voice playing the specified note and releases it.
     * If multiple voices are playing the same note, releases the oldest one.
     * 
     * @param note MIDI note number (0-127)
     */
    void releaseVoice(int note);
    
    /**
     * @brief Release all active voices
     * 
     * Useful for "all notes off" MIDI message or panic button.
     */
    void releaseAll();
    
    // ========================================================================
    // Audio Rendering
    // ========================================================================
    
    /**
     * @brief Render audio buffer from all active voices
     * 
     * Mixes the output of all active voices into the provided buffer.
     * The buffer is cleared before rendering.
     * 
     * @param buffer Output buffer to fill with audio samples
     * @param numSamples Number of samples to render
     */
    void renderBuffer(float* buffer, int numSamples);
    
    // ========================================================================
    // Voice Access
    // ========================================================================
    
    /**
     * @brief Get the number of voices in the pool
     * 
     * @return Number of voices (always 8)
     */
    int getNumVoices() const { return static_cast<int>(voices_.size()); }
    
    /**
     * @brief Get a voice by index
     * 
     * @param index Voice index (0 to 7)
     * @return Reference to the voice
     */
    Voice& getVoice(int index) { return voices_[index]; }
    
    /**
     * @brief Get a voice by index (const)
     * 
     * @param index Voice index (0 to 7)
     * @return Const reference to the voice
     */
    const Voice& getVoice(int index) const { return voices_[index]; }
    
    /**
     * @brief Get the number of currently active voices
     * 
     * @return Number of active voices (0 to 8)
     */
    int getNumActiveVoices() const;
    
    /**
     * @brief Set the sample rate for all voices
     * 
     * Call this when the audio engine sample rate changes.
     * 
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(float sampleRate);
    
    /**
     * @brief Set pitch tracking enabled/disabled for all voices
     * 
     * When pitch tracking is enabled, MIDI note numbers affect the base pitch.
     * When disabled, the base pitch parameter is used directly.
     * 
     * @param enabled true to enable pitch tracking, false to disable
     */
    void setPitchTrackingEnabled(bool enabled);
    
private:
    /**
     * @brief Find an idle voice (not active)
     * 
     * @return Pointer to idle voice, or nullptr if all voices are active
     */
    Voice* findIdleVoice();
    
    /**
     * @brief Find the oldest active voice for stealing
     * 
     * @return Pointer to oldest voice
     */
    Voice* findOldestVoice();
    
    /**
     * @brief Find a voice playing a specific note
     * 
     * @param note MIDI note number
     * @return Pointer to voice playing the note, or nullptr if not found
     */
    Voice* findVoiceByNote(int note);
    
    // Voice pool
    std::vector<Voice> voices_;
    
    // Maximum polyphony
    static constexpr int MAX_POLYPHONY = 8;
    
    // Sample rate
    float sampleRate_;
    
    // Initialization flag
    bool initialized_;
};

} // namespace KickDrum
