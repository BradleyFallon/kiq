# Design Document: Kick Drum Synthesizer

## Overview

The Kick Drum Synthesizer is a dual-format audio plugin (VST3) and standalone macOS application that generates kick drum sounds using a three-generator synthesis engine with ring modulation, dual-phase envelope system, and master effects processing. The system provides real-time waveform visualization and comprehensive parameter control for sound design.

### Key Features

- Three-generator synthesis: Sine Driver, Harmonic Membrane, Noise Generator
- Ring modulation for complex harmonic content
- Dual-phase envelope system: Warm-Up Phase and Transient/Decay Phase
- Master effects: Compressor and Reverb
- Real-time waveform visualization
- VST3 plugin and standalone macOS application
- Preset management with JSON persistence
- Low-latency audio processing

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     User Interface Layer                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  Parameter   │  │  Waveform    │  │   Preset     │      │
│  │  Controls    │  │  Display     │  │   Browser    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Audio Engine Core                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Synthesis Pipeline                       │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐    │   │
│  │  │   Sine     │  │  Harmonic  │  │   Noise    │    │   │
│  │  │  Driver    │  │  Membrane  │  │ Generator  │    │   │
│  │  └────────────┘  └────────────┘  └────────────┘    │   │
│  │         │              │                │           │   │
│  │         └──────┬───────┴────────┬───────┘           │   │
│  │                │                │                   │   │
│  │         ┌──────▼──────┐  ┌──────▼──────┐           │   │
│  │         │Ring Mod (S×H)│  │Ring Mod (S×N)│           │   │
│  │         └──────┬──────┘  └──────┬──────┘           │   │
│  │                │                │                   │   │
│  │                └────────┬───────┘                   │   │
│  │                         │                           │   │
│  │                    ┌────▼────┐                      │   │
│  │                    │  Mixer  │                      │   │
│  │                    └────┬────┘                      │   │
│  └─────────────────────────┼──────────────────────────┘   │
│                             │                              │
│  ┌─────────────────────────▼──────────────────────────┐   │
│  │              Effects Chain                          │   │
│  │  ┌────────────┐         ┌────────────┐            │   │
│  │  │ Compressor │────────▶│   Reverb   │            │   │
│  │  └────────────┘         └────────────┘            │   │
│  └─────────────────────────┬──────────────────────────┘   │
│                             │                              │
│                        ┌────▼────┐                         │
│                        │ Master  │                         │
│                        │ Output  │                         │
│                        └─────────┘                         │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Platform Layer                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  VST3 Host   │  │  CoreAudio   │  │  CoreMIDI    │      │
│  │  Interface   │  │  Interface   │  │  Interface   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

**User Interface Layer:**
- Renders parameter controls with visual feedback
- Displays real-time waveform visualization
- Manages preset browsing and selection
- Handles user input (mouse, keyboard, MIDI CC)

**Audio Engine Core:**
- Generates audio samples from synthesis pipeline
- Processes envelopes and modulation
- Applies effects chain
- Manages voice allocation and polyphony

**Platform Layer:**
- Interfaces with VST3 host or CoreAudio
- Handles MIDI input routing
- Manages audio device configuration
- Provides file system access for presets

## Components and Interfaces

### 1. Synthesis Engine

#### Sine Driver
```
class SineDriver {
  frequency: float        // Current frequency in Hz
  phase: float           // Current phase (0.0 to 1.0)
  sampleRate: float      // Audio sample rate
  
  generate() -> float    // Generate next sample
  setFrequency(freq: float)
  reset()                // Reset phase to 0
}
```

**Responsibilities:**
- Generate pure sine wave at specified frequency
- Serve as carrier for ring modulation
- Provide main tonal body of kick drum

**Implementation Notes:**
- Use phase accumulator for sample-accurate frequency control
- Apply anti-aliasing if frequency modulation is rapid
- Ensure phase continuity during frequency changes

#### Harmonic Membrane
```
class HarmonicMembrane {
  baseFrequency: float   // Reference frequency from Sine Driver
  ratio: float           // Frequency ratio (0.5x to 8.0x)
  phase: float
  sampleRate: float
  
  generate() -> float
  setRatio(ratio: float)
  setBaseFrequency(freq: float)
  reset()
}
```

**Responsibilities:**
- Generate harmonic content at frequency ratio
- Add character and tonal color
- Provide modulation source for ring modulation

**Implementation Notes:**
- Frequency = baseFrequency × ratio
- Track base frequency changes from Sine Driver
- Maintain phase continuity during ratio changes

#### Noise Generator
```
class NoiseGenerator {
  seed: uint64           // Random seed for reproducibility
  
  generate() -> float    // Generate white noise sample (-1.0 to 1.0)
  reset()                // Reset to initial seed
}
```

**Responsibilities:**
- Generate white noise for transient texture
- Provide modulation source for ring modulation

**Implementation Notes:**
- Use high-quality PRNG (e.g., xorshift)
- Ensure uniform distribution across [-1.0, 1.0]
- Optional: Support different noise colors (future enhancement)

#### Ring Modulator
```
class RingModulator {
  depth: float           // Modulation depth (0.0 to 1.0)
  
  process(carrier: float, modulator: float) -> float
  setDepth(depth: float)
}
```

**Responsibilities:**
- Multiply carrier and modulator signals
- Blend between dry and modulated signal based on depth

**Implementation:**
```
output = carrier × (1.0 - depth) + (carrier × modulator) × depth
```

**Implementation Notes:**
- Depth of 0.0 = fully dry (no modulation)
- Depth of 1.0 = fully wet (full ring modulation)
- Linear crossfade between dry and wet

### 2. Envelope System

#### Dual-Phase Envelope
```
class DualPhaseEnvelope {
  // Warm-Up Phase
  warmUpDuration: float       // 0ms to 100ms
  warmUpStartFreq: float      // 5Hz to 50Hz
  warmUpAmplitude: float      // 0.0 to 1.0
  
  // Transient/Decay Phase
  attack: float               // Attack time in seconds
  decay: float                // Decay time in seconds
  sustain: float              // Sustain level (0.0 to 1.0)
  release: float              // Release time in seconds
  
  // Curve shaping
  attackCurve: CurveType
  decayCurve: CurveType
  releaseCurve: CurveType
  
  // State
  currentPhase: Phase         // WARMUP, ATTACK, DECAY, SUSTAIN, RELEASE, IDLE
  phaseTime: float            // Time within current phase
  sampleRate: float
  
  trigger()                   // Start envelope from beginning
  release()                   // Enter release phase
  getValue() -> float         // Get current envelope value
  advance()                   // Advance by one sample
  isActive() -> bool
}
```

**Curve Types:**
```
enum CurveType {
  LINEAR,
  EXPONENTIAL,
  LOGARITHMIC,
  CUSTOM
}
```

**Curve Implementation:**
```
function applyCurve(t: float, curveType: CurveType) -> float {
  // t is normalized time (0.0 to 1.0)
  switch curveType {
    case LINEAR:
      return t
    case EXPONENTIAL:
      return t * t
    case LOGARITHMIC:
      return sqrt(t)
    case CUSTOM:
      return customCurveFunction(t)
  }
}
```

**Phase Transitions:**
1. **IDLE → WARMUP**: On trigger(), if warmUpDuration > 0
2. **WARMUP → ATTACK**: After warmUpDuration elapsed
3. **IDLE → ATTACK**: On trigger(), if warmUpDuration == 0
4. **ATTACK → DECAY**: After attack time elapsed
5. **DECAY → SUSTAIN**: After decay time elapsed
6. **SUSTAIN → RELEASE**: On release() or note-off
7. **RELEASE → IDLE**: After release time elapsed

**Implementation Notes:**
- Ensure phase continuity between WARMUP and ATTACK
- Apply curve shaping to each phase independently
- Handle retriggering during active envelope (reset to WARMUP or ATTACK)

#### Pitch Envelope
```
class PitchEnvelope {
  depth: float               // Pitch modulation depth in Hz
  envelope: DualPhaseEnvelope
  
  getValue() -> float        // Returns frequency offset in Hz
}
```

**Implementation:**
```
pitchOffset = envelope.getValue() × depth
finalFrequency = baseFrequency + pitchOffset
```

### 3. Effects Chain

#### Compressor
```
class Compressor {
  threshold: float           // Threshold in dB
  ratio: float               // Compression ratio (1.0 to 20.0)
  attack: float              // Attack time in seconds
  release: float             // Release time in seconds
  mix: float                 // Dry/wet mix (0.0 to 1.0)
  
  // State
  envelope: float            // Gain reduction envelope
  sampleRate: float
  
  process(input: float) -> float
  reset()
}
```

**Implementation Algorithm:**
```
1. Convert input to dB: inputDb = 20 × log10(abs(input))
2. Calculate gain reduction:
   if inputDb > threshold:
     gainReductionDb = (inputDb - threshold) × (1 - 1/ratio)
   else:
     gainReductionDb = 0
3. Smooth gain reduction with attack/release envelope
4. Apply gain reduction: compressed = input × 10^(-gainReduction/20)
5. Mix dry and wet: output = input × (1 - mix) + compressed × mix
```

**Implementation Notes:**
- Use ballistics (attack/release) to smooth gain reduction
- Prevent divide-by-zero when input is silent
- Apply makeup gain if needed (optional)

#### Reverb
```
class Reverb {
  roomSize: float            // Room size (0.0 to 1.0)
  decayTime: float           // Decay time in seconds
  damping: float             // High-frequency damping (0.0 to 1.0)
  mix: float                 // Dry/wet mix (0.0 to 1.0)
  
  // Internal state (implementation-specific)
  delayLines: Array<DelayLine>
  allpassFilters: Array<AllpassFilter>
  sampleRate: float
  
  process(input: float) -> float
  reset()
}
```

**Implementation Algorithm (Freeverb-style):**
```
1. Parallel comb filters (8 delay lines with feedback)
2. Series allpass filters (4 stages)
3. Apply damping to feedback paths
4. Mix dry and wet signals
```

**Implementation Notes:**
- Use prime-number delay lengths to avoid resonances
- Scale delay times based on roomSize parameter
- Apply damping as low-pass filter in feedback path
- Ensure reverb tail decays smoothly

### 4. Voice Management

```
class Voice {
  // Generators
  sineDriver: SineDriver
  harmonicMembrane: HarmonicMembrane
  noiseGenerator: NoiseGenerator
  
  // Modulators
  ringModHarmonic: RingModulator
  ringModNoise: RingModulator
  
  // Envelopes
  amplitudeEnvelope: DualPhaseEnvelope
  pitchEnvelope: PitchEnvelope
  
  // Parameters
  basePitch: float
  sineLevel: float
  harmonicLevel: float
  noiseLevel: float
  harmonicRatio: float
  harmonicModDepth: float
  noiseModDepth: float
  velocity: float
  
  trigger(note: int, velocity: float)
  release()
  isActive() -> bool
  renderSample() -> float
}
```

**Voice Rendering Algorithm:**
```
1. Advance all envelopes
2. Calculate current pitch: pitch = basePitch + pitchEnvelope.getValue()
3. Generate sine sample: sine = sineDriver.generate()
4. Generate harmonic sample: harmonic = harmonicMembrane.generate()
5. Generate noise sample: noise = noiseGenerator.generate()
6. Apply ring modulation:
   modulatedHarmonic = ringModHarmonic.process(sine, harmonic)
   modulatedNoise = ringModNoise.process(sine, noise)
7. Mix generators:
   mixed = sine × sineLevel + 
           modulatedHarmonic × harmonicLevel + 
           modulatedNoise × noiseLevel
8. Apply amplitude envelope:
   output = mixed × amplitudeEnvelope.getValue() × velocity
9. Return output
```

```
class VoiceAllocator {
  voices: Array<Voice>       // Pool of 8 voices
  maxPolyphony: int = 8
  
  allocateVoice(note: int, velocity: float) -> Voice
  releaseVoice(note: int)
  releaseAll()
  renderBuffer(buffer: Array<float>)
}
```

**Voice Allocation Strategy:**
1. Find idle voice (not active)
2. If no idle voice, steal oldest voice
3. Trigger voice with note and velocity

### 5. Parameter Management

```
class ParameterManager {
  parameters: Map<string, Parameter>
  
  registerParameter(id: string, param: Parameter)
  getParameter(id: string) -> Parameter
  setParameterValue(id: string, value: float)
  getParameterValue(id: string) -> float
  serializeToJSON() -> string
  deserializeFromJSON(json: string)
}
```

```
class Parameter {
  id: string
  name: string
  value: float
  defaultValue: float
  minValue: float
  maxValue: float
  unit: string               // "Hz", "dB", "%", "ms", etc.
  
  setValue(value: float)
  getValue() -> float
  normalize() -> float       // Return value in [0.0, 1.0]
  denormalize(norm: float)   // Set value from [0.0, 1.0]
}
```

**Parameter List:**
- Base Pitch (20Hz to 200Hz)
- Sine Driver Level (0% to 100%)
- Harmonic Ratio (0.5x to 8.0x)
- Harmonic Level (0% to 100%)
- Harmonic Mod Depth (0% to 100%)
- Noise Level (0% to 100%)
- Noise Mod Depth (0% to 100%)
- Warm-Up Duration (0ms to 100ms)
- Warm-Up Start Freq (5Hz to 50Hz)
- Warm-Up Amplitude (0% to 100%)
- Attack Time (0ms to 1000ms)
- Decay Time (0ms to 5000ms)
- Sustain Level (0% to 100%)
- Release Time (0ms to 5000ms)
- Pitch Envelope Depth (0Hz to 2000Hz)
- Attack Curve (LINEAR, EXPONENTIAL, LOGARITHMIC, CUSTOM)
- Decay Curve (LINEAR, EXPONENTIAL, LOGARITHMIC, CUSTOM)
- Release Curve (LINEAR, EXPONENTIAL, LOGARITHMIC, CUSTOM)
- Compressor Threshold (-60dB to 0dB)
- Compressor Ratio (1.0 to 20.0)
- Compressor Attack (0.1ms to 100ms)
- Compressor Release (10ms to 1000ms)
- Compressor Mix (0% to 100%)
- Reverb Room Size (0% to 100%)
- Reverb Decay Time (0.1s to 10s)
- Reverb Damping (0% to 100%)
- Reverb Mix (0% to 100%)
- Master Output Level (0% to 100%)
- Pitch Tracking (ON/OFF)

### 6. Waveform Visualization

```
class WaveformDisplay {
  waveformBuffer: Array<float>  // Rendered waveform samples
  bufferSize: int               // Number of samples to display
  zoomLevel: float              // Zoom factor (1.0 = full waveform)
  playbackPosition: int         // Current playback position
  
  updateWaveform(voice: Voice)  // Render waveform from voice
  render(graphics: Graphics)    // Draw waveform to screen
  setZoom(zoom: float)
  setPlaybackPosition(pos: int)
}
```

**Waveform Rendering:**
1. Trigger voice with current parameters
2. Render entire envelope duration to buffer
3. Downsample if needed for display
4. Draw waveform with time and amplitude axes
5. Highlight playback position if active

**Update Strategy:**
- Update on parameter change (debounced to 30fps)
- Update on MIDI trigger (show playback position)
- Use background thread for rendering to avoid UI blocking

### 7. Preset Management

```
class PresetManager {
  presets: Array<Preset>
  currentPresetIndex: int
  factoryPresets: Array<Preset>
  userPresets: Array<Preset>
  
  loadPreset(index: int)
  savePreset(name: string)
  deletePreset(index: int)
  nextPreset()
  previousPreset()
  loadFromFile(path: string) -> Preset
  saveToFile(preset: Preset, path: string)
}
```

```
class Preset {
  name: string
  version: string
  parameters: Map<string, float>
  
  toJSON() -> string
  fromJSON(json: string) -> Preset
}
```

**Preset File Format (JSON):**
```json
{
  "name": "Deep Sub Kick",
  "version": "1.0.0",
  "parameters": {
    "basePitch": 50.0,
    "sineLevel": 0.8,
    "harmonicRatio": 2.0,
    "harmonicLevel": 0.3,
    "harmonicModDepth": 0.5,
    "noiseLevel": 0.2,
    "noiseModDepth": 0.7,
    "warmUpDuration": 20.0,
    "warmUpStartFreq": 10.0,
    "warmUpAmplitude": 0.5,
    "attack": 0.001,
    "decay": 0.5,
    "sustain": 0.0,
    "release": 0.1,
    "pitchEnvelopeDepth": 500.0,
    "compressorThreshold": -12.0,
    "compressorRatio": 4.0,
    "compressorMix": 0.5,
    "reverbRoomSize": 0.3,
    "reverbDecayTime": 1.0,
    "reverbMix": 0.1,
    "masterLevel": 0.8
  }
}
```

### 8. Platform Integration

#### VST3 Plugin
```
class KickSynthVST3 : public Steinberg::Vst::AudioEffect {
  audioEngine: AudioEngine
  parameterManager: ParameterManager
  
  // VST3 interface methods
  initialize(context: FUnknown*) -> tresult
  process(data: ProcessData&) -> tresult
  setState(state: IBStream*) -> tresult
  getState(state: IBStream*) -> tresult
  setActive(state: TBool) -> tresult
}
```

**VST3 Integration:**
- Register all parameters with VST3 host
- Map VST3 parameter changes to ParameterManager
- Process MIDI events from ProcessData
- Fill audio buffers in process() callback
- Serialize/deserialize state for preset recall

#### Standalone Application (macOS)
```
class KickSynthApp {
  audioEngine: AudioEngine
  midiInput: MIDIInput
  audioOutput: AudioOutput
  userInterface: UserInterface
  
  initialize()
  run()
  shutdown()
}
```

**CoreAudio Integration:**
- Use AudioUnit or AVAudioEngine for audio output
- Configure audio device and buffer size
- Handle sample rate changes
- Process audio in real-time callback

**CoreMIDI Integration:**
- Enumerate MIDI input devices
- Register MIDI input callback
- Parse MIDI messages (note-on, note-off, CC, pitch bend)
- Route MIDI to audio engine

## Data Models

### Audio Buffer
```
struct AudioBuffer {
  samples: Array<float>      // Interleaved samples (L, R, L, R, ...)
  numChannels: int           // 1 (mono) or 2 (stereo)
  numSamples: int            // Number of frames
  sampleRate: float          // Sample rate in Hz
}
```

### MIDI Message
```
struct MIDIMessage {
  type: MIDIMessageType      // NOTE_ON, NOTE_OFF, CC, PITCH_BEND
  channel: int               // MIDI channel (0-15)
  data1: int                 // Note number or CC number
  data2: int                 // Velocity or CC value
  timestamp: uint64          // Sample-accurate timestamp
}
```

### Voice State
```
struct VoiceState {
  isActive: bool
  note: int
  velocity: float
  age: uint64                // For voice stealing
}
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*


### Property 1: Ring Modulation Multiplication

*For any* sine signal and modulator signal (harmonic or noise), when ring modulation depth is set to 100%, the output should equal the product of the sine and modulator signals.

**Validates: Requirements 1.5, 1.6**

### Property 2: Generator Mixing

*For any* set of generator levels (sine, harmonic, noise), the mixed output should equal the weighted sum of all generator outputs according to their respective level parameters.

**Validates: Requirements 1.7**

### Property 3: Audio Continuity

*For any* sequence of parameter changes during synthesis, the audio output should not contain discontinuities (sample-to-sample jumps exceeding a threshold indicating clicks or pops).

**Validates: Requirements 1.9, 12.3**

### Property 4: Phase Continuity at Envelope Transition

*For any* envelope configuration with warm-up enabled, the phase of the sine driver should be continuous (no sudden jumps) at the transition from warm-up phase to transient/decay phase.

**Validates: Requirements 2.11**

### Property 5: Velocity Scaling

*For any* MIDI velocity value, the amplitude of the synthesized kick drum should scale proportionally to the velocity (higher velocity produces proportionally louder output).

**Validates: Requirements 4.6**

### Property 6: Compression Gain Reduction

*For any* input signal with amplitude exceeding the compressor threshold, the compressor should apply gain reduction according to the specified ratio.

**Validates: Requirements 5.1**

### Property 7: Master Level Scaling

*For any* master output level setting, the final audio output should be scaled by that level (output amplitude should be proportional to master level).

**Validates: Requirements 6.2**

### Property 8: Soft Clipping

*For any* audio signal that would exceed 0dBFS (±1.0), the audio engine should apply soft clipping to keep the output within the valid range [-1.0, 1.0].

**Validates: Requirements 6.3, 15.3**

### Property 9: Plugin State Round-Trip

*For any* set of parameter values, serializing the plugin state and then deserializing it should restore all parameters to their original values.

**Validates: Requirements 8.7, 8.8**

### Property 10: Preset Round-Trip

*For any* set of parameter values, saving a preset and then loading it should restore all parameters to their original values.

**Validates: Requirements 10.1, 10.2**

### Property 11: MIDI Note Triggering

*For any* MIDI note-on message (note number 0-127, velocity 1-127), the synthesizer should trigger a new voice and begin synthesis.

**Validates: Requirements 13.1, 13.3**

### Property 12: MIDI Note-Off Envelope Continuation

*For any* active voice, receiving a MIDI note-off message should allow the amplitude envelope to continue through its release phase rather than immediately stopping the sound.

**Validates: Requirements 13.2**

### Property 13: Pitch Tracking

*For any* two different MIDI note numbers, the synthesized kick drum pitch should differ according to the note number difference (higher note number produces higher pitch).

**Validates: Requirements 13.4**

### Property 14: MIDI CC Parameter Control

*For any* MIDI CC message targeting a mapped parameter, the parameter value should change in response to the CC value.

**Validates: Requirements 13.5**

### Property 15: MIDI Pitch Bend

*For any* MIDI pitch bend message, the synthesized pitch should change according to the pitch bend value.

**Validates: Requirements 13.6**

### Property 16: JSON Serialization

*For any* valid preset, the serialized JSON should be valid JSON format and contain all parameter values.

**Validates: Requirements 14.1**

### Property 17: JSON Validation

*For any* invalid JSON input, the preset loader should reject the input and return an error without modifying the current parameter state.

**Validates: Requirements 14.2**

### Property 18: NaN/Infinity Recovery

*For any* synthesis state that produces NaN or infinity values, the audio engine should detect the invalid values and reset to a valid state without crashing.

**Validates: Requirements 15.4**

## Error Handling

### Audio Engine Errors

**Invalid Parameter Values:**
- Clamp parameters to valid ranges
- Log warning for out-of-range values
- Continue operation with clamped values

**NaN/Infinity Detection:**
- Check audio output for NaN/infinity after each processing block
- If detected, reset all oscillator phases and envelope states
- Log error with context (which component produced invalid values)
- Mute output for one buffer to prevent speaker damage

**Buffer Underrun:**
- If audio callback is called before processing is complete, output silence
- Log warning
- Continue normal operation on next callback

### File I/O Errors

**Preset Loading:**
- Validate JSON structure before parsing
- Check for required fields (name, version, parameters)
- Verify parameter names match current version
- If validation fails, display error message and keep current state
- Support migration from older preset versions

**Preset Saving:**
- Check for write permissions before saving
- If save fails, display error message with reason
- Retry with user-selected location if needed

### MIDI Errors

**Device Disconnection:**
- Detect MIDI device disconnection
- Continue audio processing without MIDI input
- Attempt reconnection every 5 seconds
- Display status in UI

**Invalid MIDI Messages:**
- Ignore malformed MIDI messages
- Log warning with message details
- Continue processing valid messages

### Audio Device Errors

**Initialization Failure:**
- Display error message with device name and error code
- Allow user to select different audio device
- Retry initialization with new device

**Sample Rate Change:**
- Detect sample rate change from host/system
- Reinitialize audio engine with new sample rate
- Preserve parameter values during reinitialization
- Clear audio buffers to prevent artifacts

## Testing Strategy

### Dual Testing Approach

The testing strategy employs both unit tests and property-based tests to ensure comprehensive coverage:

**Unit Tests:**
- Verify specific examples and edge cases
- Test integration points between components
- Validate error handling paths
- Test UI interactions and visual feedback

**Property-Based Tests:**
- Verify universal properties across all inputs
- Use randomized input generation for comprehensive coverage
- Run minimum 100 iterations per property test
- Each property test references its design document property

### Property-Based Testing Configuration

**Library Selection:**
- Use fast-check (JavaScript/TypeScript) or QuickCheck-style library
- Configure for minimum 100 iterations per test
- Use shrinking to find minimal failing cases

**Test Tagging:**
Each property test must include a comment tag:
```
// Feature: kick-drum-synthesizer, Property 1: Ring Modulation Multiplication
```

### Unit Test Focus Areas

**Specific Examples:**
- Warm-up phase with 0ms duration (bypass)
- Compressor with threshold at 0dB (no compression)
- Reverb with 0% mix (dry signal)
- Envelope with all times set to 0 (instant)

**Edge Cases:**
- Maximum polyphony (8 voices)
- Rapid parameter changes
- Extreme parameter values (min/max ranges)
- Empty preset files
- Corrupted JSON

**Integration Tests:**
- VST3 host communication
- CoreAudio device initialization
- CoreMIDI device enumeration
- Preset file I/O
- UI responsiveness

### Performance Testing

**Latency Benchmarks:**
- Measure audio processing time for 512-sample buffer at 48kHz
- Target: < 10ms processing time
- Test on reference hardware (2020 M1 MacBook Pro)

**CPU Usage:**
- Measure CPU usage during sustained synthesis
- Target: < 5% on reference hardware
- Test with maximum polyphony (8 voices)

**Memory Usage:**
- Monitor memory allocation during operation
- Verify no memory leaks over extended use
- Test preset loading/unloading

### Test Coverage Goals

- 90%+ code coverage for audio engine core
- 100% coverage of error handling paths
- All 18 correctness properties implemented as property tests
- All edge cases covered by unit tests
- Performance benchmarks passing on reference hardware

## Implementation Notes

### Audio Processing Considerations

**Sample-Accurate Timing:**
- Process MIDI events at exact sample positions within buffer
- Use fractional phase accumulation for oscillators
- Interpolate parameter changes within buffer

**Anti-Aliasing:**
- Apply oversampling for ring modulation (2x or 4x)
- Use band-limited oscillators if frequency modulation is rapid
- Filter noise generator output if needed

**Denormal Prevention:**
- Add small DC offset to prevent denormals in feedback paths
- Use flush-to-zero mode if available on platform
- Monitor CPU usage for denormal-related spikes

### UI/UX Considerations

**Responsiveness:**
- Update waveform display at 30fps minimum
- Debounce parameter changes to reduce CPU load
- Use background thread for waveform rendering
- Provide visual feedback within 16ms (60fps)

**Preset Management:**
- Auto-save current state on application close
- Provide undo/redo for parameter changes
- Display preset modified indicator
- Support drag-and-drop for preset files

### Platform-Specific Notes

**macOS:**
- Use Accelerate framework for DSP operations
- Support Retina displays for UI rendering
- Handle audio device hot-plugging
- Support dark mode

**VST3:**
- Implement parameter automation correctly
- Support sample-accurate automation
- Handle host tempo/transport if needed
- Provide proper latency reporting

## Future Enhancements

### Potential Features

**Additional Generators:**
- Triangle/square wave oscillators
- Filtered noise (pink, brown)
- Sample-based layer

**Modulation:**
- LFOs for parameter modulation
- Envelope followers
- Step sequencer for rhythmic modulation

**Effects:**
- Distortion/saturation
- EQ (parametric or graphic)
- Transient shaper
- Stereo widening

**Preset Management:**
- Preset categories and tags
- Preset search and filtering
- Cloud sync for presets
- Preset sharing/marketplace

**Visualization:**
- Spectrum analyzer
- Oscilloscope
- Envelope visualizer
- Modulation matrix display

### Scalability Considerations

**Performance:**
- SIMD optimization for DSP operations
- Multi-threaded audio processing
- GPU acceleration for effects
- Adaptive quality based on CPU load

**Extensibility:**
- Plugin architecture for effects
- Modular synthesis components
- Scriptable parameter mapping
- MIDI learn for any parameter
