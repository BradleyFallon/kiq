# MIDI Pitch Bend Implementation

## Overview

This document describes the implementation of MIDI pitch bend functionality for the kick drum synthesizer (Task 13.9). The implementation allows pitch bend messages to modulate the base pitch of all active voices with configurable pitch bend range.

## Requirements

**Task 13.9: Implement MIDI pitch bend**
- Apply pitch bend to base pitch
- Implement pitch bend range control
- Requirements: 13.6

**Requirement 13.6:** THE Kick_Synth SHALL respond to MIDI pitch bend messages

## Implementation Details

### 1. MIDIHandler Class Updates

#### New Member Variables
```cpp
float currentPitchBend_;    // Current pitch bend value [-1.0, 1.0]
float pitchBendRange_;      // Pitch bend range in semitones (default ±2)
```

#### New Methods

**handlePitchBend(int lsb, int msb)**
- Combines 7-bit LSB and MSB into 14-bit pitch bend value
- Normalizes to [-1.0, 1.0] range (0.0 = center, 8192 = center value)
- Applies pitch bend to all active voices
- Updates currentPitchBend_ state

**setPitchBendRange(float semitones)**
- Sets the pitch bend range in semitones
- Clamped to [0.0, 24.0] semitones
- Default is 2.0 semitones (±2 semitones)

**getPitchBendRange() const**
- Returns the current pitch bend range

**getCurrentPitchBend() const**
- Returns the current pitch bend value [-1.0, 1.0]

#### Updated Methods

**processMIDIMessage(const MIDIMessage& message)**
- Now handles PITCH_BEND message type
- Routes to handlePitchBend()

**handleNoteOn(int note, int velocity)**
- Applies current pitch bend state to newly allocated voices
- Ensures new voices inherit the current pitch bend

### 2. Voice Class Updates

#### New Member Variables
```cpp
float pitchBendValue_;      // Current pitch bend value (-1.0 to 1.0)
float pitchBendRange_;      // Pitch bend range in semitones
```

#### New Methods

**setPitchBend(float bendValue, float bendRange)**
- Sets the pitch bend value and range for the voice
- Stores both parameters for use during rendering

**getPitchBendValue() const**
- Returns the current pitch bend value

**getPitchBendRange() const**
- Returns the current pitch bend range

#### Updated Methods

**renderSample()**
- Applies pitch bend to the base pitch during rendering
- Formula: `pitchBendSemitones = pitchBendValue * pitchBendRange`
- Converts semitones to frequency ratio: `ratio = 2^(semitones/12)`
- Applies to base pitch: `finalPitch = (basePitch + pitchEnvelope) * pitchBendRatio`
- Pitch bend combines multiplicatively with pitch envelope (additive)

### 3. Pitch Bend Calculation

The pitch bend is applied using the standard musical formula:

```cpp
// Calculate pitch bend in semitones
float pitchBendSemitones = pitchBendValue_ * pitchBendRange_;

// Convert semitones to frequency ratio
float pitchBendRatio = std::pow(2.0f, pitchBendSemitones / 12.0f);

// Apply to base pitch (after pitch envelope)
float currentPitch = (basePitch_ + pitchOffset) * pitchBendRatio;
```

**Examples:**
- Pitch bend value = 0.0 (center): ratio = 1.0 (no change)
- Pitch bend value = 1.0, range = 2.0: ratio = 2^(2/12) ≈ 1.122 (+2 semitones)
- Pitch bend value = -1.0, range = 2.0: ratio = 2^(-2/12) ≈ 0.891 (-2 semitones)
- Pitch bend value = 1.0, range = 12.0: ratio = 2.0 (+1 octave)

### 4. MIDI Pitch Bend Message Format

MIDI pitch bend uses 14-bit resolution:
- LSB (data1): Lower 7 bits (0-127)
- MSB (data2): Upper 7 bits (0-127)
- Combined value: `(MSB << 7) | LSB` = 0-16383
- Center value: 8192 (no pitch bend)
- Normalized range: [-1.0, 1.0]

### 5. Integration with Existing Systems

**Pitch Envelope:**
- Pitch bend is applied multiplicatively after pitch envelope
- Pitch envelope adds frequency offset in Hz
- Pitch bend multiplies the result by a frequency ratio
- Both can be active simultaneously

**Voice Allocation:**
- New voices automatically inherit current pitch bend state
- Ensures consistent pitch across all voices
- Pitch bend is global (affects all active voices)

**MIDI Message Processing:**
- Pitch bend messages are processed in processMIDIMessage()
- Routed to handlePitchBend() method
- Applied immediately to all active voices

## Testing

### Unit Tests

**MIDIHandler Tests (19 tests):**
- HandlePitchBendCenterValue: Verifies center value (8192) = 0.0
- HandlePitchBendMaxUp: Verifies max up (16383) = 1.0
- HandlePitchBendMaxDown: Verifies max down (0) = -1.0
- HandlePitchBendHalfUp/Down: Verifies intermediate values
- HandlePitchBendWithNullVoiceAllocator: Error handling
- HandlePitchBendAppliedToAllActiveVoices: Multi-voice support
- ProcessMIDIMessagePitchBend: Message routing
- SetPitchBendRange: Range configuration
- SetPitchBendRangeClampMin/Max: Range clamping
- DefaultPitchBendRange/Value: Default values
- PitchBendRangeAppliedToVoices: Range propagation
- Integration tests: Complex scenarios

**Voice Tests (15 tests):**
- PitchBendDefaultValues: Default state
- SetPitchBendCenter/MaxUp/MaxDown: Value setting
- SetPitchBendWithDifferentRange: Range configuration
- PitchBendAffectsRenderedPitch: Rendering integration
- PitchBendUpIncreasesFrequency: Frequency increase
- PitchBendDownDecreasesFrequency: Frequency decrease
- PitchBendWithLargeRange: Large range (12 semitones)
- PitchBendWithZeroRange: Zero range (no effect)
- PitchBendCombinesWithPitchEnvelope: Envelope interaction
- PitchBendPersistsAcrossMultipleSamples: State persistence
- PitchBendCanBeChangedDuringRendering: Dynamic changes
- PitchBendWithExtremeValues: Extreme values (24 semitones)
- PitchBendDoesNotAffectInactiveVoice: Inactive voice handling

**MIDIMessage Tests (6 tests):**
- ParsePitchBend: Message parsing
- PitchBendValueCenter/MaxUp/MaxDown: Value extraction
- PitchBendValueHalfwayUp: Intermediate values
- PitchBendValueNonPitchBend: Non-pitch-bend messages

### Test Results

All 40 pitch bend tests pass:
- 15 Voice tests
- 19 MIDIHandler tests
- 6 MIDIMessage tests

All 94 MIDI-related tests pass (including existing tests).

## Usage Example

```cpp
// Create MIDI handler
MIDIHandler midiHandler(&voiceAllocator, &parameterManager);

// Set pitch bend range to 12 semitones (one octave)
midiHandler.setPitchBendRange(12.0f);

// Trigger a note
midiHandler.handleNoteOn(60, 100);

// Apply pitch bend (half up = +6 semitones)
midiHandler.handlePitchBend(0, 96);  // LSB=0, MSB=96 → value=0.5

// Or process a MIDI message
MIDIMessage pitchBendMsg(MIDIMessageType::PITCH_BEND, 0, 0, 96, 0);
midiHandler.processMIDIMessage(pitchBendMsg);

// Get current pitch bend state
float currentBend = midiHandler.getCurrentPitchBend();  // 0.5
float bendRange = midiHandler.getPitchBendRange();     // 12.0
```

## Design Decisions

1. **Global Pitch Bend:** Pitch bend affects all active voices simultaneously, which is standard MIDI behavior.

2. **Multiplicative Application:** Pitch bend is applied multiplicatively to the pitch (after pitch envelope), which preserves musical intervals correctly.

3. **Default Range:** Default pitch bend range is ±2 semitones, which is the MIDI standard.

4. **Range Clamping:** Pitch bend range is clamped to [0, 24] semitones to prevent extreme values.

5. **State Inheritance:** New voices inherit the current pitch bend state to ensure consistency.

6. **14-bit Resolution:** Full 14-bit MIDI pitch bend resolution is supported for smooth pitch changes.

## Files Modified

- `src/audio_engine/midi/MIDIHandler.h`
- `src/audio_engine/midi/MIDIHandler.cpp`
- `src/audio_engine/voice/Voice.h`
- `src/audio_engine/voice/Voice.cpp`
- `tests/unit/midi/MIDIHandlerTest.cpp`
- `tests/unit/voice/VoiceTest.cpp`

## Files Created

- `test_pitch_bend.sh` - Test script for pitch bend functionality
- `MIDI_PITCH_BEND_IMPLEMENTATION.md` - This document

## Validation

✅ Task 13.9 complete:
- Pitch bend applied to base pitch
- Pitch bend range control implemented
- Requirement 13.6 validated

✅ All tests pass:
- 40 new pitch bend tests
- 94 total MIDI tests
- 58 total Voice tests
- No regressions in existing tests

## Future Enhancements

1. **Per-Voice Pitch Bend:** Support for polyphonic pitch bend (MPE)
2. **Pitch Bend Smoothing:** Add smoothing to prevent zipper noise
3. **Pitch Bend Reset:** Add method to reset pitch bend to center
4. **Pitch Bend Visualization:** Display pitch bend in UI
5. **Pitch Bend Recording:** Record pitch bend in preset system
