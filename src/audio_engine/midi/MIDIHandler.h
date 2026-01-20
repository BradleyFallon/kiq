#pragma once

#include "MIDIMessage.h"
#include "../voice/VoiceAllocator.h"
#include "../parameters/ParameterManager.h"
#include <map>
#include <string>

namespace KickDrum {

/**
 * @brief MIDIHandler routes MIDI messages to the voice allocator and parameters
 * 
 * The MIDIHandler class processes MIDI messages and routes them to the
 * appropriate destinations:
 * 
 * - NOTE_ON: Allocates a voice and triggers synthesis with velocity
 * - NOTE_OFF: Releases the voice playing the specified note
 * - CC: Updates mapped parameters based on CC mappings
 * 
 * MIDI CC Mapping:
 * - CC messages can be mapped to synthesis parameters
 * - CC values (0-127) are normalized to parameter ranges
 * - CC learn mode allows dynamic mapping of controllers
 * - Multiple CC controllers can control different parameters
 * 
 * Velocity Handling:
 * - MIDI velocity (0-127) is normalized to [0.0, 1.0]
 * - Velocity is applied to amplitude in the Voice class
 * - Note-on with velocity 0 is treated as note-off
 * 
 * Requirements validated:
 * - 13.1: MIDI note-on triggers synthesis with velocity affecting amplitude
 * - 13.2: MIDI note-off allows envelope to complete naturally
 * - 13.5: MIDI CC messages control parameters
 */
class MIDIHandler {
public:
    /**
     * @brief Construct a new MIDIHandler
     * 
     * @param voiceAllocator Pointer to the voice allocator to route messages to
     * @param parameterManager Pointer to the parameter manager for CC mapping (optional)
     */
    explicit MIDIHandler(VoiceAllocator* voiceAllocator, ParameterManager* parameterManager = nullptr);
    
    /**
     * @brief Process a MIDI message
     * 
     * Routes the MIDI message to the appropriate handler:
     * - NOTE_ON: Calls handleNoteOn()
     * - NOTE_OFF: Calls handleNoteOff()
     * - CC: Calls handleCC()
     * - Other message types are currently ignored
     * 
     * @param message The MIDI message to process
     */
    void processMIDIMessage(const MIDIMessage& message);
    
    /**
     * @brief Handle a MIDI note-on message
     * 
     * Allocates a voice and triggers synthesis:
     * 1. Normalizes velocity from [0-127] to [0.0-1.0]
     * 2. Allocates a voice from the voice allocator
     * 3. Voice is triggered with note number and normalized velocity
     * 
     * Note: Note-on with velocity 0 is treated as note-off
     * 
     * @param note MIDI note number (0-127)
     * @param velocity MIDI velocity (0-127)
     */
    void handleNoteOn(int note, int velocity);
    
    /**
     * @brief Handle a MIDI note-off message
     * 
     * Releases the voice playing the specified note:
     * 1. Finds the voice playing the note
     * 2. Calls release() on the voice
     * 3. Voice envelope enters release phase and completes naturally
     * 
     * @param note MIDI note number (0-127)
     */
    void handleNoteOff(int note);
    
    /**
     * @brief Handle a MIDI CC message
     * 
     * If the CC number is mapped to a parameter:
     * 1. Normalizes CC value from [0-127] to [0.0-1.0]
     * 2. Updates the mapped parameter via ParameterManager
     * 
     * If CC learn mode is active:
     * 1. Maps the CC number to the learn target parameter
     * 2. Disables CC learn mode
     * 
     * @param ccNumber CC controller number (0-127)
     * @param ccValue CC value (0-127)
     */
    void handleCC(int ccNumber, int ccValue);
    
    /**
     * @brief Handle a MIDI pitch bend message
     * 
     * Applies pitch bend to all active voices:
     * 1. Extracts pitch bend value from data bytes (14-bit)
     * 2. Normalizes to [-1.0, 1.0] range (0.0 = center)
     * 3. Applies pitch bend to all active voices
     * 
     * The pitch bend range is controlled by the pitchBendRange parameter
     * (typically ±2 semitones by default).
     * 
     * @param lsb Pitch bend LSB (0-127)
     * @param msb Pitch bend MSB (0-127)
     */
    void handlePitchBend(int lsb, int msb);
    
    /**
     * @brief Map a CC controller to a parameter
     * 
     * Creates a mapping from a MIDI CC number to a parameter ID.
     * When the CC is received, the parameter will be updated.
     * 
     * @param ccNumber CC controller number (0-127)
     * @param parameterId Parameter ID to control
     * @return true if mapping successful, false if parameter doesn't exist
     */
    bool mapCCToParameter(int ccNumber, const std::string& parameterId);
    
    /**
     * @brief Unmap a CC controller
     * 
     * Removes the mapping for the specified CC number.
     * 
     * @param ccNumber CC controller number (0-127)
     */
    void unmapCC(int ccNumber);
    
    /**
     * @brief Clear all CC mappings
     */
    void clearAllCCMappings();
    
    /**
     * @brief Get the parameter ID mapped to a CC number
     * 
     * @param ccNumber CC controller number (0-127)
     * @return Parameter ID, or empty string if not mapped
     */
    std::string getMappedParameter(int ccNumber) const;
    
    /**
     * @brief Check if a CC number is mapped
     * 
     * @param ccNumber CC controller number (0-127)
     * @return true if mapped, false otherwise
     */
    bool isCCMapped(int ccNumber) const;
    
    /**
     * @brief Enable CC learn mode
     * 
     * When CC learn mode is active, the next CC message received
     * will be mapped to the specified parameter.
     * 
     * @param parameterId Parameter ID to learn
     * @return true if parameter exists, false otherwise
     */
    bool enableCCLearn(const std::string& parameterId);
    
    /**
     * @brief Disable CC learn mode
     */
    void disableCCLearn();
    
    /**
     * @brief Check if CC learn mode is active
     * 
     * @return true if CC learn is active, false otherwise
     */
    bool isCCLearnActive() const { return ccLearnActive_; }
    
    /**
     * @brief Get the parameter ID for CC learn
     * 
     * @return Parameter ID being learned, or empty string if not active
     */
    std::string getCCLearnParameter() const { return ccLearnParameterId_; }
    
    /**
     * @brief Get all CC mappings
     * 
     * @return Map of CC numbers to parameter IDs
     */
    std::map<int, std::string> getAllCCMappings() const { return ccMappings_; }
    
    /**
     * @brief Set the voice allocator
     * 
     * @param voiceAllocator Pointer to the voice allocator
     */
    void setVoiceAllocator(VoiceAllocator* voiceAllocator);
    
    /**
     * @brief Get the voice allocator
     * 
     * @return Pointer to the voice allocator
     */
    VoiceAllocator* getVoiceAllocator() const { return voiceAllocator_; }
    
    /**
     * @brief Set the parameter manager
     * 
     * @param parameterManager Pointer to the parameter manager
     */
    void setParameterManager(ParameterManager* parameterManager);
    
    /**
     * @brief Get the parameter manager
     * 
     * @return Pointer to the parameter manager
     */
    ParameterManager* getParameterManager() const { return parameterManager_; }
    
    /**
     * @brief Set the pitch bend range in semitones
     * 
     * Controls how much pitch bend affects the pitch.
     * Typical values are ±2 semitones (default) or ±12 semitones (one octave).
     * 
     * @param semitones Pitch bend range in semitones (0.0 to 24.0)
     */
    void setPitchBendRange(float semitones);
    
    /**
     * @brief Get the pitch bend range in semitones
     * 
     * @return Pitch bend range in semitones
     */
    float getPitchBendRange() const { return pitchBendRange_; }
    
    /**
     * @brief Get the current pitch bend value
     * 
     * @return Current pitch bend value in range [-1.0, 1.0]
     */
    float getCurrentPitchBend() const { return currentPitchBend_; }
    
private:
    /**
     * @brief Normalize MIDI velocity to [0.0, 1.0]
     * 
     * @param velocity MIDI velocity (0-127)
     * @return Normalized velocity (0.0-1.0)
     */
    float normalizeVelocity(int velocity) const;
    
    /**
     * @brief Normalize MIDI CC value to [0.0, 1.0]
     * 
     * @param ccValue MIDI CC value (0-127)
     * @return Normalized CC value (0.0-1.0)
     */
    float normalizeCCValue(int ccValue) const;
    
    // Voice allocator for routing MIDI messages
    VoiceAllocator* voiceAllocator_;
    
    // Parameter manager for CC mapping
    ParameterManager* parameterManager_;
    
    // CC mappings: CC number -> Parameter ID
    std::map<int, std::string> ccMappings_;
    
    // CC learn mode state
    bool ccLearnActive_;
    std::string ccLearnParameterId_;
    
    // Pitch bend state
    float currentPitchBend_;    // Current pitch bend value [-1.0, 1.0]
    float pitchBendRange_;      // Pitch bend range in semitones (default ±2)
};

} // namespace KickDrum
