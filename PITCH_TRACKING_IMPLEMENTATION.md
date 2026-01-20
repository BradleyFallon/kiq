# Pitch Tracking Implementation

## Overview

This document describes the implementation of pitch tracking functionality for the kick drum synthesizer (Task 13.3). Pitch tracking allows MIDI note numbers to affect the base pitch of the synthesizer, enabling melodic kick drum patterns.

## Requirements Validated

- **Requirement 4.7**: Pitch tracking parameter allowing MIDI note number to affect base pitch
- **Requirement 13.4**: Different MIDI note numbers adjust base pitch accordingly

## Implementation Details

### 1. MIDI Note to Frequency Conversion (DSPUtils.h)

Added a utility function to convert MIDI note numbers to frequencies using the standard MIDI tuning formula:

```cpp
inline float midiNoteToFrequency(int note) {
    // Standard MIDI tuning: A4 (note 69) = 440 Hz
    // Formula: f = 440 * 2^((note - 69) / 12)
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}
```

**Key Features:**
- Uses standard MIDI tuning where A4 (note 69) = 440 Hz
- Supports full MIDI range (0-127)
- Inline function for optimal performance

**Examples:**
- MIDI note 60 (C4) = 261.63 Hz
- MIDI note 36 (C2) = 65.41 Hz (typical kick drum range)
- MIDI note 24 (C1) = 32.70 Hz (low kick drum)

### 2. Voice Class Modifications (Voice.h/cpp)

Added pitch tracking support to the Voice class:

#### New Methods:

```cpp
void setPitchTrackingEnabled(bool enabled);
bool isPitchTrackingEnabled() const;
void setPitchFromMIDINote(int note);
```

#### New State Variable:

```cpp
bool pitchTrackingEnabled_;  // Pitch tracking enable/disable
```

#### Behavior:

1. **Pitch Tracking Enabled (default):**
   - When a voice is triggered, the MIDI note number is converted to a frequency
   - The base pitch is set to this frequency
   - Different MIDI notes produce different pitches

2. **Pitch Tracking Disabled:**
   - The base pitch parameter is used directly
   - MIDI note numbers do not affect pitch
   - All notes use the same base pitch

#### Implementation in trigger():

```cpp
void Voice::trigger(int note, float velocity) {
    note_ = note;
    velocity_ = velocity;
    age_ = 0;
    
    // If pitch tracking is enabled, set pitch from MIDI note
    if (pitchTrackingEnabled_) {
        setPitchFromMIDINote(note);
    }
    
    // Reset generators and trigger envelopes...
}
```

### 3. VoiceAllocator Integration (VoiceAllocator.h/cpp)

Added a method to set pitch tracking for all voices in the pool:

```cpp
void VoiceAllocator::setPitchTrackingEnabled(bool enabled) {
    // Update all voices with pitch tracking state
    for (auto& voice : voices_) {
        voice.setPitchTrackingEnabled(enabled);
    }
}
```

This allows the audio engine to control pitch tracking globally for all voices.

## Testing

### Test Coverage

Created comprehensive tests in `test_pitch_tracking_compile.sh`:

1. **MIDI Note to Frequency Conversion**
   - Validates standard MIDI tuning (A4 = 440 Hz)
   - Tests common kick drum notes (C1, C2, C3)
   - Verifies octave relationships

2. **Pitch Tracking Enable/Disable**
   - Tests default state (enabled)
   - Tests toggling pitch tracking on/off

3. **Pitch Tracking Affects Base Pitch**
   - Verifies different MIDI notes produce different pitches
   - Tests typical kick drum range (C1-C3)

4. **Pitch Tracking Disabled**
   - Verifies base pitch remains constant when disabled
   - Tests with multiple MIDI notes

5. **VoiceAllocator Integration**
   - Tests setting pitch tracking for all voices
   - Verifies allocated voices use MIDI pitch when enabled
   - Verifies allocated voices ignore MIDI pitch when disabled

### Test Results

All tests pass successfully:

```
=== All Pitch Tracking Tests Passed! ===

Requirements validated:
  ✓ 4.7: Pitch tracking parameter allowing MIDI note to affect base pitch
  ✓ 13.4: Different MIDI note numbers adjust base pitch accordingly
```

## Usage Example

### Enabling Pitch Tracking

```cpp
// Create and initialize voice allocator
VoiceAllocator allocator;
allocator.initialize(48000.0f);

// Enable pitch tracking for all voices
allocator.setPitchTrackingEnabled(true);

// Allocate voices with different MIDI notes
Voice* voice1 = allocator.allocateVoice(36, 1.0f);  // C2 = 65.41 Hz
Voice* voice2 = allocator.allocateVoice(48, 1.0f);  // C3 = 130.81 Hz
Voice* voice3 = allocator.allocateVoice(60, 1.0f);  // C4 = 261.63 Hz
```

### Disabling Pitch Tracking

```cpp
// Disable pitch tracking
allocator.setPitchTrackingEnabled(false);

// Set a specific base pitch for all voices
for (int i = 0; i < allocator.getNumVoices(); ++i) {
    allocator.getVoice(i).setBasePitch(50.0f);  // All voices use 50 Hz
}

// All allocated voices will use 50 Hz regardless of MIDI note
Voice* voice = allocator.allocateVoice(60, 1.0f);  // Still uses 50 Hz
```

## Integration with Parameter System

The pitch tracking parameter is already registered in ParameterManager:

```cpp
// Pitch Tracking (0 = OFF, 1 = ON)
registerParameter(Parameter("pitchTracking", "Pitch Tracking", 1.0f, 0.0f, 1.0f, ""));
```

To integrate with the audio engine:

```cpp
// In AudioEngine or similar
float pitchTrackingValue = parameterManager->getParameterValue("pitchTracking");
bool pitchTrackingEnabled = (pitchTrackingValue > 0.5f);
voiceAllocator->setPitchTrackingEnabled(pitchTrackingEnabled);
```

## Typical Kick Drum MIDI Note Ranges

| MIDI Note | Note Name | Frequency | Use Case |
|-----------|-----------|-----------|----------|
| 24 | C1 | 32.70 Hz | Very low, sub-bass kick |
| 36 | C2 | 65.41 Hz | Typical kick drum |
| 48 | C3 | 130.81 Hz | High kick, tom-like |
| 60 | C4 | 261.63 Hz | Very high, percussion |

## Design Decisions

1. **Default State: Enabled**
   - Pitch tracking is enabled by default to provide the most intuitive behavior
   - Users expect MIDI notes to affect pitch in most synthesizers

2. **Per-Voice Control**
   - Each voice has its own pitch tracking state
   - Allows for future enhancements (e.g., per-voice pitch tracking)

3. **Standard MIDI Tuning**
   - Uses the industry-standard MIDI tuning formula
   - Ensures compatibility with other instruments and DAWs

4. **Inline Utility Function**
   - The conversion function is inline for optimal performance
   - Called frequently during voice triggering

## Future Enhancements

Potential improvements for future versions:

1. **Pitch Bend Support**
   - Add pitch bend modulation on top of base pitch
   - Implement pitch bend range parameter

2. **Microtuning**
   - Support alternative tuning systems
   - Allow custom frequency tables

3. **Pitch Quantization**
   - Snap pitches to specific scales
   - Useful for melodic kick patterns

4. **Pitch Envelope Scaling**
   - Scale pitch envelope depth based on MIDI note
   - Higher notes could have less pitch sweep

## Files Modified

1. **src/audio_engine/utils/DSPUtils.h**
   - Added `midiNoteToFrequency()` utility function

2. **src/audio_engine/voice/Voice.h**
   - Added pitch tracking methods and state variable
   - Updated class documentation

3. **src/audio_engine/voice/Voice.cpp**
   - Implemented pitch tracking methods
   - Modified `trigger()` to apply pitch tracking
   - Added DSPUtils include

4. **src/audio_engine/voice/VoiceAllocator.h**
   - Added `setPitchTrackingEnabled()` method

5. **src/audio_engine/voice/VoiceAllocator.cpp**
   - Implemented `setPitchTrackingEnabled()` method

## Testing Files

1. **test_pitch_tracking_compile.sh**
   - Comprehensive test script for pitch tracking functionality
   - Tests all requirements and edge cases

2. **tests/unit/voice/PitchTrackingTest.cpp**
   - Google Test-based unit tests (for future integration)

## Conclusion

The pitch tracking implementation successfully enables MIDI note numbers to affect the base pitch of the kick drum synthesizer. The implementation is clean, well-tested, and follows the existing codebase patterns. All requirements (4.7 and 13.4) are validated through comprehensive testing.
