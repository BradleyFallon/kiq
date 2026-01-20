# MIDI Handler Implementation

## Overview

This document describes the implementation of task 13.2: MIDI note handling for the kick drum synthesizer.

## Implementation Summary

The `MIDIHandler` class routes MIDI messages to the voice allocator, handling note-on and note-off events:

### Key Features

1. **Note-On Handling**
   - Routes MIDI note-on messages to the voice allocator
   - Normalizes MIDI velocity (0-127) to float range (0.0-1.0)
   - Allocates a voice and triggers synthesis with the normalized velocity
   - Handles velocity 0 as note-off (per MIDI specification)

2. **Note-Off Handling**
   - Routes MIDI note-off messages to the voice allocator
   - Releases the voice playing the specified note
   - Allows the amplitude envelope to complete naturally (release phase)

3. **Velocity Application**
   - Velocity is normalized from MIDI range [0-127] to [0.0-1.0]
   - Applied to amplitude in the Voice class during rendering
   - Higher velocity produces proportionally louder output

## Files Created/Modified

### New Files
- `src/audio_engine/midi/MIDIHandler.h` - MIDIHandler class declaration
- `src/audio_engine/midi/MIDIHandler.cpp` - MIDIHandler implementation
- `tests/unit/midi/MIDIHandlerTest.cpp` - Comprehensive unit tests

## Class Interface

### MIDIHandler

```cpp
class MIDIHandler {
public:
    explicit MIDIHandler(VoiceAllocator* voiceAllocator);
    
    void processMIDIMessage(const MIDIMessage& message);
    void handleNoteOn(int note, int velocity);
    void handleNoteOff(int note);
    
    void setVoiceAllocator(VoiceAllocator* voiceAllocator);
    VoiceAllocator* getVoiceAllocator() const;
    
private:
    float normalizeVelocity(int velocity) const;
    VoiceAllocator* voiceAllocator_;
};
```

## Implementation Details

### Note-On Processing

1. Receives MIDI note-on message with note number (0-127) and velocity (0-127)
2. Normalizes velocity: `normalizedVelocity = velocity / 127.0f`
3. Calls `voiceAllocator->allocateVoice(note, normalizedVelocity)`
4. Voice allocator finds an idle voice or steals the oldest voice
5. Voice is triggered with the note and normalized velocity

### Note-Off Processing

1. Receives MIDI note-off message with note number
2. Calls `voiceAllocator->releaseVoice(note)`
3. Voice allocator finds the voice playing that note
4. Voice enters release phase of amplitude envelope
5. Envelope completes naturally (no abrupt cutoff)

### Velocity Normalization

```cpp
float normalizeVelocity(int velocity) const {
    int clampedVelocity = std::max(0, std::min(127, velocity));
    return static_cast<float>(clampedVelocity) / 127.0f;
}
```

- Clamps velocity to valid MIDI range [0-127]
- Normalizes to [0.0-1.0] for use in audio processing
- Handles out-of-range values gracefully

## Test Coverage

The implementation includes 30 comprehensive unit tests covering:

### Constructor Tests (2 tests)
- Constructor with valid voice allocator
- Constructor with null voice allocator

### Note-On Tests (8 tests)
- Voice allocation on note-on
- Correct note number assignment
- Velocity normalization
- Zero velocity handling
- Min/max velocity handling
- Null voice allocator handling

### Note-Off Tests (3 tests)
- Voice release on note-off
- Non-existent note handling
- Null voice allocator handling

### MIDI Message Processing Tests (6 tests)
- Note-on message processing
- Note-off message processing
- Note-on with velocity 0 (treated as note-off)
- CC message ignored
- Pitch bend message ignored
- Unknown message ignored

### Multiple Note Tests (4 tests)
- Multiple simultaneous notes
- Note-on/off sequences
- Full polyphony (8 voices)
- Voice stealing on polyphony limit

### Edge Case Tests (4 tests)
- Negative velocity (clamped to 0)
- Velocity above 127 (clamped to 127)
- Minimum note number (0)
- Maximum note number (127)

### Voice Allocator Setter Tests (2 tests)
- Setting new voice allocator
- Setting null voice allocator

### Integration Tests (2 tests)
- Complete note-on/off cycle
- Multiple notes with different velocities

## Requirements Validated

This implementation validates the following requirements:

- **Requirement 13.1**: MIDI note-on triggers synthesis with velocity affecting amplitude
- **Requirement 13.2**: MIDI note-off allows envelope to complete naturally

## Usage Example

```cpp
// Create voice allocator
VoiceAllocator voiceAllocator;
voiceAllocator.initialize(44100.0f);

// Create MIDI handler
MIDIHandler midiHandler(&voiceAllocator);

// Process MIDI messages
MIDIMessage noteOn(MIDIMessageType::NOTE_ON, 0, 60, 100, 0);
midiHandler.processMIDIMessage(noteOn);

// Or handle directly
midiHandler.handleNoteOn(60, 100);  // Middle C, velocity 100
midiHandler.handleNoteOff(60);      // Release middle C
```

## Integration with Audio Engine

The MIDIHandler is designed to be integrated into the AudioEngine class:

1. AudioEngine receives MIDI messages from the host/platform
2. AudioEngine passes messages to MIDIHandler
3. MIDIHandler routes to VoiceAllocator
4. VoiceAllocator manages voice triggering and release
5. Voices render audio samples with velocity-scaled amplitude

## Future Enhancements

The following MIDI features are planned for future tasks:

- **Task 13.5**: Pitch tracking (MIDI note number affects base pitch)
- **Task 13.7**: MIDI CC mapping to parameters
- **Task 13.9**: MIDI pitch bend handling

These features will be added to the MIDIHandler in subsequent tasks.

## Test Results

All 30 unit tests pass successfully:

```
[==========] Running 30 tests from 1 test suite.
[----------] 30 tests from MIDIHandlerTest
[  PASSED  ] 30 tests.
```

## Conclusion

Task 13.2 is complete. The MIDIHandler successfully routes MIDI note-on and note-off messages to the voice allocator, with proper velocity normalization and envelope handling. The implementation is well-tested and ready for integration into the audio engine.
