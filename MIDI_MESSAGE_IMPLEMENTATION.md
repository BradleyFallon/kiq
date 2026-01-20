# MIDI Message Parsing Implementation

## Overview

This document describes the implementation of MIDI message parsing for the Kick Drum Synthesizer (Task 13.1). The implementation provides a robust MIDI message representation and parsing functionality that extracts channel, data bytes, and timestamps from raw MIDI messages.

## Requirements Addressed

- **Requirement 13.1**: Parse MIDI note-on messages
- **Requirement 13.2**: Parse MIDI note-off messages
- **Requirement 13.5**: Parse MIDI CC (Control Change) messages
- **Requirement 13.6**: Parse MIDI pitch bend messages

## Implementation Details

### Files Created

1. **src/audio_engine/midi/MIDIMessage.h** - Header file with MIDIMessage structure and parsing functions
2. **src/audio_engine/midi/MIDIMessage.cpp** - Implementation of MIDI message parsing
3. **tests/unit/midi/MIDIMessageTest.cpp** - Comprehensive unit tests (22 tests)

### MIDIMessage Structure

```cpp
struct MIDIMessage {
    MIDIMessageType type;   // NOTE_ON, NOTE_OFF, CC, PITCH_BEND, UNKNOWN
    int channel;            // MIDI channel (0-15)
    int data1;              // First data byte (0-127)
    int data2;              // Second data byte (0-127)
    uint64_t timestamp;     // Sample-accurate timestamp
};
```

### Supported Message Types

1. **Note-On (0x90-0x9F)**
   - data1: Note number (0-127)
   - data2: Velocity (0-127)
   - Special case: Velocity 0 is treated as note-off

2. **Note-Off (0x80-0x8F)**
   - data1: Note number (0-127)
   - data2: Release velocity (0-127)

3. **Control Change (0xB0-0xBF)**
   - data1: Controller number (0-127)
   - data2: Controller value (0-127)

4. **Pitch Bend (0xE0-0xEF)**
   - data1: LSB (0-127)
   - data2: MSB (0-127)
   - Combined into 14-bit value (0-16383)
   - Normalized to [-1.0, 1.0] range

### Key Features

#### 1. Channel Extraction
- Extracts MIDI channel from lower 4 bits of status byte
- Supports all 16 MIDI channels (0-15)

#### 2. Data Byte Masking
- Masks data bytes to 7 bits for safety
- Prevents invalid MIDI data from causing issues

#### 3. Pitch Bend Normalization
- Combines 7-bit LSB and MSB into 14-bit value
- Center value (8192) maps to 0.0
- Range 0-16383 maps to [-1.0, 1.0]

#### 4. Note-On/Note-Off Detection
- `isNoteOn()`: Returns true for note-on with velocity > 0
- `isNoteOff()`: Returns true for note-off OR note-on with velocity 0

#### 5. Timestamp Support
- 64-bit timestamp for sample-accurate timing
- Preserved through parsing

### Parsing Functions

```cpp
// Parse from individual bytes
MIDIMessage parseMIDIMessage(uint8_t status, uint8_t data1, uint8_t data2, uint64_t timestamp = 0);

// Parse from byte array
MIDIMessage parseMIDIMessage(const uint8_t* bytes, uint64_t timestamp = 0);
```

## Test Coverage

### Unit Tests (22 tests)

1. **Constructor Tests**
   - Default constructor
   - Parameterized constructor

2. **Parsing Tests**
   - Note-on messages (various channels)
   - Note-off messages
   - Note-on with velocity 0 (treated as note-off)
   - Control change messages
   - Pitch bend messages
   - Unknown message types

3. **Pitch Bend Tests**
   - Center position (0.0)
   - Maximum up (+1.0)
   - Maximum down (-1.0)
   - Halfway up (~0.5)
   - Non-pitch-bend messages (returns 0.0)

4. **Edge Case Tests**
   - All MIDI channels (0-15)
   - All note numbers (0-127)
   - All velocity values (0-127)
   - Data byte masking (safety)
   - Null byte array handling
   - Timestamp preservation

### Test Results

All 22 tests pass successfully:
```
[==========] Running 22 tests from 1 test suite.
[  PASSED  ] 22 tests.
```

## Usage Examples

### Parsing a Note-On Message

```cpp
// Raw MIDI bytes: Note-on, channel 0, note 60 (middle C), velocity 100
uint8_t status = 0x90;
uint8_t data1 = 60;
uint8_t data2 = 100;
uint64_t timestamp = 1000;

MIDIMessage msg = parseMIDIMessage(status, data1, data2, timestamp);

// Check message type
if (msg.isNoteOn()) {
    int note = msg.data1;      // 60
    int velocity = msg.data2;  // 100
    int channel = msg.channel; // 0
    // Trigger synthesis...
}
```

### Parsing from Byte Array

```cpp
uint8_t midiBytes[3] = {0x90, 60, 100};
MIDIMessage msg = parseMIDIMessage(midiBytes, timestamp);
```

### Handling Pitch Bend

```cpp
uint8_t status = 0xE0;  // Pitch bend, channel 0
uint8_t lsb = 0x00;
uint8_t msb = 0x60;     // Halfway up

MIDIMessage msg = parseMIDIMessage(status, lsb, msb);

if (msg.type == MIDIMessageType::PITCH_BEND) {
    float bendValue = msg.getPitchBendValue();  // ~0.5
    // Apply pitch bend...
}
```

### Handling Control Change

```cpp
uint8_t status = 0xB0;  // CC, channel 0
uint8_t ccNum = 7;      // Volume
uint8_t ccValue = 100;

MIDIMessage msg = parseMIDIMessage(status, ccNum, ccValue);

if (msg.type == MIDIMessageType::CC) {
    int controller = msg.data1;  // 7
    int value = msg.data2;       // 100
    // Map to parameter...
}
```

## Design Decisions

### 1. Struct vs Class
- Used `struct` for MIDIMessage as it's primarily a data container
- Added helper methods for convenience (isNoteOn, isNoteOff, getPitchBendValue)

### 2. Separate Parsing Functions
- Free functions for parsing keep the interface clean
- Allows parsing from different input formats (bytes, arrays)

### 3. Data Byte Masking
- Always mask data bytes to 7 bits for safety
- Prevents invalid MIDI data from causing issues

### 4. Note-On Velocity 0
- Follows MIDI standard: note-on with velocity 0 = note-off
- Both `isNoteOn()` and `isNoteOff()` handle this correctly

### 5. Pitch Bend Normalization
- Normalized to [-1.0, 1.0] for easier use in synthesis
- Center value (8192) maps to 0.0
- Allows direct multiplication with pitch bend range

## Integration Points

### Current Integration
- Included in audio engine library (CMakeLists.txt)
- Unit tests integrated into test suite

### Future Integration (Upcoming Tasks)
- **Task 13.2**: MIDI note handling (route to voice allocator)
- **Task 13.3**: Pitch tracking (map note number to base pitch)
- **Task 13.7**: MIDI CC mapping (map CC to parameters)
- **Task 13.9**: MIDI pitch bend (apply to base pitch)

## Building and Testing

### Build Tests
```bash
cmake --build build --target kick_drum_tests
```

### Run MIDI Message Tests
```bash
./build/bin/kick_drum_tests --gtest_filter="MIDIMessageTest.*"
```

### Run Test Script
```bash
./test_midi_message.sh
```

## Next Steps

1. **Task 13.2**: Implement MIDI note handling
   - Route note-on to voice allocator
   - Route note-off to voice release
   - Apply velocity to amplitude

2. **Task 13.3**: Implement pitch tracking
   - Map MIDI note number to base pitch
   - Implement pitch tracking enable/disable

3. **Task 13.7**: Implement MIDI CC mapping
   - Map CC messages to parameters
   - Implement CC learn functionality

4. **Task 13.9**: Implement MIDI pitch bend
   - Apply pitch bend to base pitch
   - Implement pitch bend range control

## Conclusion

The MIDI message parsing implementation provides a solid foundation for MIDI input handling in the Kick Drum Synthesizer. It correctly parses all required message types (note-on, note-off, CC, pitch bend), extracts channel and data information, and provides convenient helper methods for common operations. All 22 unit tests pass, demonstrating robust handling of various MIDI messages and edge cases.
