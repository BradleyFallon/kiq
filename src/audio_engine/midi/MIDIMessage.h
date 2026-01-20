#pragma once

#include <cstdint>

namespace KickDrum {

/**
 * @brief MIDI message types supported by the synthesizer
 */
enum class MIDIMessageType {
    NOTE_ON,        ///< Note-on message (status 0x90-0x9F)
    NOTE_OFF,       ///< Note-off message (status 0x80-0x8F)
    CC,             ///< Control Change message (status 0xB0-0xBF)
    PITCH_BEND,     ///< Pitch bend message (status 0xE0-0xEF)
    UNKNOWN         ///< Unknown or unsupported message type
};

/**
 * @brief MIDI message representation
 * 
 * Represents a parsed MIDI message with type, channel, data bytes, and timestamp.
 * This structure is used throughout the audio engine for MIDI event processing.
 * 
 * Data byte interpretation depends on message type:
 * - NOTE_ON/NOTE_OFF: data1 = note number (0-127), data2 = velocity (0-127)
 * - CC: data1 = controller number (0-127), data2 = controller value (0-127)
 * - PITCH_BEND: data1 = LSB (0-127), data2 = MSB (0-127)
 */
struct MIDIMessage {
    MIDIMessageType type;   ///< Message type
    int channel;            ///< MIDI channel (0-15)
    int data1;              ///< First data byte (0-127)
    int data2;              ///< Second data byte (0-127)
    uint64_t timestamp;     ///< Sample-accurate timestamp
    
    /**
     * @brief Construct a default MIDI message
     */
    MIDIMessage();
    
    /**
     * @brief Construct a MIDI message with all fields
     * @param type Message type
     * @param channel MIDI channel (0-15)
     * @param data1 First data byte (0-127)
     * @param data2 Second data byte (0-127)
     * @param timestamp Sample-accurate timestamp
     */
    MIDIMessage(MIDIMessageType type, int channel, int data1, int data2, uint64_t timestamp = 0);
    
    /**
     * @brief Check if this is a note-on message with non-zero velocity
     * @return true if this is a valid note-on, false otherwise
     * 
     * Note: MIDI note-on with velocity 0 is treated as note-off
     */
    bool isNoteOn() const;
    
    /**
     * @brief Check if this is a note-off message (or note-on with velocity 0)
     * @return true if this is a note-off, false otherwise
     */
    bool isNoteOff() const;
    
    /**
     * @brief Get the pitch bend value as a normalized float
     * @return Pitch bend value in range [-1.0, 1.0], where 0.0 is center
     * 
     * Combines data1 (LSB) and data2 (MSB) into a 14-bit value,
     * then normalizes to [-1.0, 1.0] range.
     */
    float getPitchBendValue() const;
};

/**
 * @brief Parse a raw MIDI message from bytes
 * @param status Status byte (includes message type and channel)
 * @param data1 First data byte
 * @param data2 Second data byte
 * @param timestamp Sample-accurate timestamp
 * @return Parsed MIDIMessage structure
 * 
 * Parses standard MIDI messages:
 * - Note-on: 0x90-0x9F
 * - Note-off: 0x80-0x8F
 * - Control Change: 0xB0-0xBF
 * - Pitch Bend: 0xE0-0xEF
 * 
 * The channel is extracted from the lower 4 bits of the status byte.
 * Data bytes are masked to 7 bits (0-127) for safety.
 */
MIDIMessage parseMIDIMessage(uint8_t status, uint8_t data1, uint8_t data2, uint64_t timestamp = 0);

/**
 * @brief Parse a raw MIDI message from a byte array
 * @param bytes Pointer to MIDI message bytes (minimum 3 bytes)
 * @param timestamp Sample-accurate timestamp
 * @return Parsed MIDIMessage structure
 * 
 * Convenience function that extracts status and data bytes from an array.
 */
MIDIMessage parseMIDIMessage(const uint8_t* bytes, uint64_t timestamp = 0);

} // namespace KickDrum
