#pragma once

#include "../generators/SineDriver.h"
#include "../generators/HarmonicMembrane.h"
#include "../generators/NoiseGenerator.h"
#include "../modulation/RingModulator.h"
#include "../envelopes/DualPhaseEnvelope.h"
#include "../envelopes/PitchEnvelope.h"

namespace KickDrum {

/**
 * @brief Voice class integrating generators, modulators, and envelopes
 * 
 * The Voice class represents a single synthesis voice that combines:
 * - Three generators: Sine Driver, Harmonic Membrane, Noise Generator
 * - Two ring modulators: for harmonic and noise modulation
 * - Amplitude envelope: dual-phase envelope for amplitude control
 * - Pitch envelope: for pitch modulation
 * 
 * The voice rendering algorithm:
 * 1. Advance all envelopes
 * 2. Calculate current pitch with pitch envelope modulation
 * 3. Generate samples from all three generators
 * 4. Apply ring modulation to harmonic and noise
 * 5. Mix generators with their respective levels
 * 6. Apply amplitude envelope and velocity scaling
 * 
 * Requirements validated:
 * - 1.1: Three-generator synthesis with ring modulation
 * - 4.6: Velocity scaling to amplitude
 */
class Voice {
public:
    /**
     * @brief Construct a new Voice
     */
    Voice();
    
    /**
     * @brief Initialize the voice with a sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     */
    void initialize(float sampleRate);
    
    // ========================================================================
    // Voice Control
    // ========================================================================
    
    /**
     * @brief Trigger the voice with a MIDI note
     * 
     * @param note MIDI note number (0-127)
     * @param velocity MIDI velocity (0.0 to 1.0)
     */
    void trigger(int note, float velocity);
    
    /**
     * @brief Release the voice
     * 
     * Transitions the amplitude envelope to the release phase.
     */
    void release();
    
    /**
     * @brief Check if the voice is currently active
     * 
     * @return true if the voice is generating audio
     */
    bool isActive() const;
    
    /**
     * @brief Get the MIDI note number for this voice
     * 
     * @return MIDI note number (0-127)
     */
    int getNote() const { return note_; }
    
    /**
     * @brief Get the voice age (for voice stealing)
     * 
     * @return Number of samples since voice was triggered
     */
    uint64_t getAge() const { return age_; }
    
    // ========================================================================
    // Synthesis Parameters
    // ========================================================================
    
    /**
     * @brief Set the base pitch
     * 
     * @param pitch Base pitch in Hz (20Hz to 200Hz)
     */
    void setBasePitch(float pitch);
    
    /**
     * @brief Get the base pitch
     * 
     * @return Base pitch in Hz
     */
    float getBasePitch() const { return basePitch_; }
    
    /**
     * @brief Set the pitch tracking enable/disable
     * 
     * When pitch tracking is enabled, the MIDI note number affects the base pitch.
     * When disabled, the base pitch parameter is used directly.
     * 
     * @param enabled true to enable pitch tracking, false to disable
     */
    void setPitchTrackingEnabled(bool enabled);
    
    /**
     * @brief Get the pitch tracking enabled state
     * 
     * @return true if pitch tracking is enabled
     */
    bool isPitchTrackingEnabled() const { return pitchTrackingEnabled_; }
    
    /**
     * @brief Set the pitch from a MIDI note number
     * 
     * This is called when pitch tracking is enabled. The MIDI note number
     * is converted to a frequency using the standard MIDI tuning formula.
     * 
     * @param note MIDI note number (0-127)
     */
    void setPitchFromMIDINote(int note);
    
    /**
     * @brief Set the pitch bend amount
     * 
     * Applies pitch bend modulation to the base pitch.
     * The pitch bend value is in the range [-1.0, 1.0] where 0.0 is center.
     * The range parameter controls how many semitones the pitch bend affects.
     * 
     * @param bendValue Pitch bend value (-1.0 to 1.0, 0.0 = center)
     * @param bendRange Pitch bend range in semitones (e.g., 2.0 for ±2 semitones)
     */
    void setPitchBend(float bendValue, float bendRange);
    
    /**
     * @brief Get the current pitch bend value
     * 
     * @return Pitch bend value (-1.0 to 1.0)
     */
    float getPitchBendValue() const { return pitchBendValue_; }
    
    /**
     * @brief Get the current pitch bend range
     * 
     * @return Pitch bend range in semitones
     */
    float getPitchBendRange() const { return pitchBendRange_; }
    
    /**
     * @brief Set the sine driver level
     * 
     * @param level Level (0.0 to 1.0)
     */
    void setSineLevel(float level);
    
    /**
     * @brief Get the sine driver level
     * 
     * @return Level (0.0 to 1.0)
     */
    float getSineLevel() const { return sineLevel_; }
    
    /**
     * @brief Set the harmonic level
     * 
     * @param level Level (0.0 to 1.0)
     */
    void setHarmonicLevel(float level);
    
    /**
     * @brief Get the harmonic level
     * 
     * @return Level (0.0 to 1.0)
     */
    float getHarmonicLevel() const { return harmonicLevel_; }
    
    /**
     * @brief Set the noise level
     * 
     * @param level Level (0.0 to 1.0)
     */
    void setNoiseLevel(float level);
    
    /**
     * @brief Get the noise level
     * 
     * @return Level (0.0 to 1.0)
     */
    float getNoiseLevel() const { return noiseLevel_; }
    
    /**
     * @brief Set the harmonic frequency ratio
     * 
     * @param ratio Frequency ratio (0.5x to 8.0x)
     */
    void setHarmonicRatio(float ratio);
    
    /**
     * @brief Get the harmonic frequency ratio
     * 
     * @return Frequency ratio
     */
    float getHarmonicRatio() const { return harmonicRatio_; }
    
    /**
     * @brief Set the harmonic modulation depth
     * 
     * @param depth Modulation depth (0.0 to 1.0)
     */
    void setHarmonicModDepth(float depth);
    
    /**
     * @brief Get the harmonic modulation depth
     * 
     * @return Modulation depth (0.0 to 1.0)
     */
    float getHarmonicModDepth() const { return harmonicModDepth_; }
    
    /**
     * @brief Set the noise modulation depth
     * 
     * @param depth Modulation depth (0.0 to 1.0)
     */
    void setNoiseModDepth(float depth);
    
    /**
     * @brief Get the noise modulation depth
     * 
     * @return Modulation depth (0.0 to 1.0)
     */
    float getNoiseModDepth() const { return noiseModDepth_; }
    
    // ========================================================================
    // Envelope Access
    // ========================================================================
    
    /**
     * @brief Get access to the amplitude envelope
     * 
     * @return Reference to the amplitude envelope
     */
    DualPhaseEnvelope& getAmplitudeEnvelope() { return amplitudeEnvelope_; }
    
    /**
     * @brief Get const access to the amplitude envelope
     * 
     * @return Const reference to the amplitude envelope
     */
    const DualPhaseEnvelope& getAmplitudeEnvelope() const { return amplitudeEnvelope_; }
    
    /**
     * @brief Get access to the pitch envelope
     * 
     * @return Reference to the pitch envelope
     */
    PitchEnvelope& getPitchEnvelope() { return pitchEnvelope_; }
    
    /**
     * @brief Get const access to the pitch envelope
     * 
     * @return Const reference to the pitch envelope
     */
    const PitchEnvelope& getPitchEnvelope() const { return pitchEnvelope_; }
    
    // ========================================================================
    // Audio Rendering
    // ========================================================================
    
    /**
     * @brief Render the next audio sample
     * 
     * This implements the voice rendering algorithm:
     * 1. Advance all envelopes
     * 2. Calculate current pitch with pitch envelope modulation
     * 3. Generate samples from all three generators
     * 4. Apply ring modulation to harmonic and noise
     * 5. Mix generators with their respective levels
     * 6. Apply amplitude envelope and velocity scaling
     * 
     * @return Audio sample in range approximately [-1.0, 1.0]
     */
    float renderSample();
    
    /**
     * @brief Set the sample rate
     * 
     * Call this when the audio engine sample rate changes.
     * 
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(float sampleRate);
    
private:
    // Generators
    SineDriver sineDriver_;
    HarmonicMembrane harmonicMembrane_;
    NoiseGenerator noiseGenerator_;
    
    // Modulators
    RingModulator ringModHarmonic_;
    RingModulator ringModNoise_;
    
    // Envelopes
    DualPhaseEnvelope amplitudeEnvelope_;
    PitchEnvelope pitchEnvelope_;
    
    // Parameters
    float basePitch_;           // Base pitch in Hz (20-200)
    float sineLevel_;           // Sine driver level (0.0-1.0)
    float harmonicLevel_;       // Harmonic level (0.0-1.0)
    float noiseLevel_;          // Noise level (0.0-1.0)
    float harmonicRatio_;       // Harmonic frequency ratio (0.5-8.0)
    float harmonicModDepth_;    // Harmonic modulation depth (0.0-1.0)
    float noiseModDepth_;       // Noise modulation depth (0.0-1.0)
    bool pitchTrackingEnabled_; // Pitch tracking enable/disable
    float pitchBendValue_;      // Current pitch bend value (-1.0 to 1.0)
    float pitchBendRange_;      // Pitch bend range in semitones
    
    // Voice state
    int note_;                  // MIDI note number (0-127)
    float velocity_;            // MIDI velocity (0.0-1.0)
    uint64_t age_;              // Voice age in samples (for voice stealing)
    float sampleRate_;          // Sample rate in Hz
    bool initialized_;          // Initialization flag
};

} // namespace KickDrum
