#include "MIDIMessage.h"

namespace KickDrum {

// Default constructor
MIDIMessage::MIDIMessage()
    : type(MIDIMessageType::UNKNOWN)
    , channel(0)
    , data1(0)
    , data2(0)
    , timestamp(0)
{
}

// Parameterized constructor
MIDIMessage::MIDIMessage(MIDIMessageType type, int channel, int data1, int data2, uint64_t timestamp)
    : type(type)
    , channel(channel)
    , data1(data1)
    , data2(data2)
    , timestamp(timestamp)
{
}

// Check if this is a note-on message with non-zero velocity
bool MIDIMessage::isNoteOn() const {
    return type == MIDIMessageType::NOTE_ON && data2 > 0;
}

// Check if this is a note-off message (or note-on with velocity 0)
bool MIDIMessage::isNoteOff() const {
    return type == MIDIMessageType::NOTE_OFF || 
           (type == MIDIMessageType::NOTE_ON && data2 == 0);
}

// Get the pitch bend value as a normalized float
float MIDIMessage::getPitchBendValue() const {
    if (type != MIDIMessageType::PITCH_BEND) {
        return 0.0f;
    }
    
    // Combine 7-bit LSB and MSB into 14-bit value
    int pitchBend14bit = (data2 << 7) | data1;
    
    // Center value is 8192 (0x2000)
    // Range is 0-16383 (0x0000-0x3FFF)
    const int centerValue = 8192;
    const float maxRange = 8192.0f;
    
    // Normalize to [-1.0, 1.0]
    return (pitchBend14bit - centerValue) / maxRange;
}

// Parse a raw MIDI message from bytes
MIDIMessage parseMIDIMessage(uint8_t status, uint8_t data1, uint8_t data2, uint64_t timestamp) {
    // Extract message type from upper 4 bits
    uint8_t messageType = status & 0xF0;
    
    // Extract channel from lower 4 bits (0-15)
    int channel = status & 0x0F;
    
    // Mask data bytes to 7 bits for safety
    int dataByte1 = data1 & 0x7F;
    int dataByte2 = data2 & 0x7F;
    
    // Determine message type
    MIDIMessageType type = MIDIMessageType::UNKNOWN;
    
    switch (messageType) {
        case 0x80:  // Note-off
            type = MIDIMessageType::NOTE_OFF;
            break;
            
        case 0x90:  // Note-on
            type = MIDIMessageType::NOTE_ON;
            break;
            
        case 0xB0:  // Control Change
            type = MIDIMessageType::CC;
            break;
            
        case 0xE0:  // Pitch Bend
            type = MIDIMessageType::PITCH_BEND;
            break;
            
        default:
            type = MIDIMessageType::UNKNOWN;
            break;
    }
    
    return MIDIMessage(type, channel, dataByte1, dataByte2, timestamp);
}

// Parse a raw MIDI message from a byte array
MIDIMessage parseMIDIMessage(const uint8_t* bytes, uint64_t timestamp) {
    if (bytes == nullptr) {
        return MIDIMessage();
    }
    
    return parseMIDIMessage(bytes[0], bytes[1], bytes[2], timestamp);
}

} // namespace KickDrum
