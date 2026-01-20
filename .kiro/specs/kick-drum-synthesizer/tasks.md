# Implementation Plan: Kick Drum Synthesizer

## Overview

This implementation plan breaks down the kick drum synthesizer into discrete coding tasks. The implementation will use C++ for the audio engine (industry standard for VST plugins) with platform-specific UI frameworks. Each task builds incrementally, with testing integrated throughout.

## Tasks

- [x] 1. Set up project structure and build system
  - Create directory structure for audio engine, UI, and platform layers
  - Configure CMake build system for cross-platform compilation
  - Set up VST3 SDK integration
  - Configure CoreAudio and CoreMIDI frameworks for macOS
  - Set up testing framework (Google Test for C++, fast-check for property tests)
  - _Requirements: 7.1, 8.1_

- [ ] 2. Implement core synthesis generators
  - [x] 2.1 Implement Sine Driver oscillator
    - Write SineDriver class with phase accumulator
    - Implement frequency control and phase reset
    - Ensure sample-accurate frequency changes
    - _Requirements: 1.1, 1.2_
  
  - [ ]* 2.2 Write property test for Sine Driver
    - **Property: Sine wave frequency accuracy**
    - **Validates: Requirements 1.1**
  
  - [x] 2.3 Implement Harmonic Membrane oscillator
    - Write HarmonicMembrane class with frequency ratio control
    - Track base frequency from Sine Driver
    - Implement ratio parameter (0.5x to 8.0x)
    - _Requirements: 1.1, 1.3, 3.1_
  
  - [ ]* 2.4 Write property test for Harmonic Membrane
    - **Property: Harmonic frequency ratio accuracy**
    - **Validates: Requirements 3.1**
  
  - [x] 2.5 Implement Noise Generator
    - Write NoiseGenerator class with PRNG (xorshift)
    - Generate uniform white noise in [-1.0, 1.0]
    - Implement seed control for reproducibility
    - _Requirements: 1.1, 1.4_
  
  - [ ]* 2.6 Write unit tests for Noise Generator
    - Test noise distribution uniformity
    - Test seed reproducibility
    - _Requirements: 1.4_

- [ ] 3. Implement ring modulation and mixing
  - [x] 3.1 Implement Ring Modulator
    - Write RingModulator class with depth control
    - Implement carrier × modulator multiplication
    - Implement dry/wet blending based on depth
    - _Requirements: 1.5, 1.6, 3.2, 3.3_
  
  - [ ]* 3.2 Write property test for ring modulation
    - **Property 1: Ring Modulation Multiplication**
    - **Validates: Requirements 1.5, 1.6**
  
  - [ ]* 3.3 Write unit tests for modulation depth edge cases
    - Test 0% depth (fully dry)
    - Test 100% depth (fully wet)
    - _Requirements: 3.4, 3.5, 3.6, 3.7_
  
  - [x] 3.4 Implement generator mixer
    - Write mixer that combines sine, modulated harmonics, and modulated noise
    - Implement independent level controls for each source
    - _Requirements: 1.7, 4.2, 4.3, 4.4_
  
  - [ ]* 3.5 Write property test for generator mixing
    - **Property 2: Generator Mixing**
    - **Validates: Requirements 1.7**

- [x] 4. Checkpoint - Ensure generator tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 5. Implement dual-phase envelope system
  - [x] 5.1 Implement envelope curve functions
    - Write curve functions (linear, exponential, logarithmic, custom)
    - Implement curve application to normalized time
    - _Requirements: 2.6, 2.8, 2.9_
  
  - [x] 5.2 Implement Warm-Up Phase envelope
    - Write warm-up phase with duration, start frequency, and amplitude controls
    - Implement frequency sweep from start frequency to base pitch
    - _Requirements: 2.1, 2.3, 2.4, 2.5_
  
  - [x] 5.3 Implement Transient/Decay Phase envelope
    - Write ADSR envelope with attack, decay, sustain, release
    - Implement phase transitions (ATTACK → DECAY → SUSTAIN → RELEASE)
    - Apply curve shaping to each phase
    - _Requirements: 2.2, 2.6, 2.7, 2.8, 2.9_
  
  - [x] 5.4 Implement DualPhaseEnvelope coordinator
    - Coordinate warm-up and transient/decay phases
    - Ensure phase continuity at transition
    - Handle trigger and release events
    - _Requirements: 2.1, 2.2, 2.11_
  
  - [ ]* 5.5 Write property test for phase continuity
    - **Property 4: Phase Continuity at Envelope Transition**
    - **Validates: Requirements 2.11**
  
  - [x] 5.6 Implement Pitch Envelope
    - Write PitchEnvelope class wrapping DualPhaseEnvelope
    - Apply depth parameter to envelope output
    - Return frequency offset in Hz
    - _Requirements: 2.7_
  
  - [ ]* 5.7 Write unit tests for envelope edge cases
    - Test warm-up with 0ms duration (bypass)
    - Test envelope with all times set to 0
    - Test envelope retriggering
    - _Requirements: 2.3, 2.6_

- [ ] 6. Implement voice management
  - [x] 6.1 Implement Voice class
    - Integrate generators, ring modulators, and envelopes
    - Implement voice rendering algorithm
    - Handle trigger and release events
    - Apply velocity scaling to amplitude
    - _Requirements: 1.1, 4.6_
  
  - [ ]* 6.2 Write property test for velocity scaling
    - **Property 5: Velocity Scaling**
    - **Validates: Requirements 4.6**
  
  - [x] 6.3 Implement VoiceAllocator
    - Create voice pool (8 voices)
    - Implement voice allocation strategy (find idle or steal oldest)
    - Implement voice release and rendering
    - _Requirements: 12.4_
  
  - [ ]* 6.4 Write unit test for polyphony
    - Test 8 simultaneous voices
    - Test voice stealing when all voices active
    - _Requirements: 12.4_
  
  - [ ]* 6.5 Write property test for audio continuity
    - **Property 3: Audio Continuity**
    - **Validates: Requirements 1.9, 12.3**

- [x] 7. Checkpoint - Ensure voice tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 8. Implement effects chain
  - [x] 8.1 Implement Compressor
    - Write Compressor class with threshold, ratio, attack, release
    - Implement gain reduction calculation
    - Apply ballistics (attack/release envelope)
    - Implement dry/wet mix
    - _Requirements: 5.1, 5.3, 5.4_
  
  - [ ]* 8.2 Write property test for compression
    - **Property 6: Compression Gain Reduction**
    - **Validates: Requirements 5.1**
  
  - [x] 8.3 Implement Reverb
    - Write Reverb class using Freeverb algorithm
    - Implement parallel comb filters and series allpass filters
    - Implement room size, decay time, and damping controls
    - Implement dry/wet mix
    - _Requirements: 5.2, 5.5, 5.6_
  
  - [ ]* 8.4 Write unit tests for reverb
    - Test reverb tail decay
    - Test dry/wet mixing
    - _Requirements: 5.5, 5.6_
  
  - [x] 8.5 Implement effects chain coordinator
    - Chain compressor → reverb in series
    - Implement bypass controls for each effect
    - _Requirements: 5.1, 5.2, 5.8, 5.9_
  
  - [ ]* 8.6 Write unit test for effects chain order
    - Verify compressor is applied before reverb
    - _Requirements: 5.2_

- [ ] 9. Implement master output and safety
  - [x] 9.1 Implement master output level control
    - Apply master level scaling after effects
    - _Requirements: 6.1, 6.2_
  
  - [ ]* 9.2 Write property test for master level scaling
    - **Property 7: Master Level Scaling**
    - **Validates: Requirements 6.2**
  
  - [x] 9.3 Implement soft clipping
    - Detect signals exceeding ±1.0
    - Apply soft clipping algorithm (tanh or cubic)
    - _Requirements: 6.3, 15.3_
  
  - [ ]* 9.4 Write property test for soft clipping
    - **Property 8: Soft Clipping**
    - **Validates: Requirements 6.3, 15.3**
  
  - [x] 9.5 Implement NaN/infinity detection and recovery
    - Check audio output for invalid values
    - Reset synthesis state if detected
    - Log error with context
    - _Requirements: 15.4_
  
  - [ ]* 9.6 Write property test for NaN/infinity recovery
    - **Property 18: NaN/Infinity Recovery**
    - **Validates: Requirements 15.4**

- [x] 10. Checkpoint - Ensure effects and safety tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 11. Implement parameter management
  - [x] 11.1 Implement Parameter class
    - Write Parameter class with value, range, and unit
    - Implement normalization and denormalization
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.7_
  
  - [x] 11.2 Implement ParameterManager
    - Create parameter registry
    - Register all synthesis parameters
    - Implement parameter value get/set
    - _Requirements: 4.1-4.7, 5.3-5.6_
  
  - [x] 11.3 Implement JSON serialization for parameters
    - Serialize parameters to JSON
    - Deserialize parameters from JSON
    - Include version information
    - _Requirements: 14.1, 14.4_
  
  - [ ]* 11.4 Write property test for JSON serialization
    - **Property 16: JSON Serialization**
    - **Validates: Requirements 14.1**
  
  - [ ]* 11.5 Write property test for JSON validation
    - **Property 17: JSON Validation**
    - **Validates: Requirements 14.2**
  
  - [ ]* 11.6 Write unit test for version information
    - Verify version field exists in serialized JSON
    - _Requirements: 14.4_

- [ ] 12. Implement preset management
  - [x] 12.1 Implement Preset class
    - Write Preset class with name, version, and parameters
    - Implement toJSON and fromJSON methods
    - _Requirements: 10.1, 10.2_
  
  - [x] 12.2 Implement PresetManager
    - Manage preset list (factory and user presets)
    - Implement load, save, delete, next, previous
    - Implement file I/O with .kdpreset extension
    - _Requirements: 10.1, 10.2, 10.3, 10.5, 10.6, 14.5_
  
  - [ ]* 12.3 Write property test for preset round-trip
    - **Property 10: Preset Round-Trip**
    - **Validates: Requirements 10.1, 10.2**
  
  - [ ]* 12.4 Write unit tests for preset error handling
    - Test corrupted preset file
    - Test invalid JSON
    - Verify state unchanged on error
    - _Requirements: 14.3_
  
  - [ ]* 12.5 Write unit test for preset file extension
    - Verify .kdpreset extension is used
    - _Requirements: 14.5_

- [ ] 13. Implement MIDI handling
  - [x] 13.1 Implement MIDI message parsing
    - Parse MIDI note-on, note-off, CC, pitch bend messages
    - Extract channel, data1, data2, timestamp
    - _Requirements: 13.1, 13.2, 13.5, 13.6_
  
  - [x] 13.2 Implement MIDI note handling
    - Route note-on to voice allocator
    - Route note-off to voice release
    - Apply velocity to amplitude
    - _Requirements: 13.1, 13.2_
  
  - [ ]* 13.3 Write property test for MIDI note triggering
    - **Property 11: MIDI Note Triggering**
    - **Validates: Requirements 13.1, 13.3**
  
  - [ ]* 13.4 Write property test for MIDI note-off
    - **Property 12: MIDI Note-Off Envelope Continuation**
    - **Validates: Requirements 13.2**
  
  - [x] 13.3 Implement pitch tracking
    - Map MIDI note number to base pitch
    - Implement pitch tracking enable/disable
    - _Requirements: 4.7, 13.4_
  
  - [ ]* 13.6 Write property test for pitch tracking
    - **Property 13: Pitch Tracking**
    - **Validates: Requirements 13.4**
  
  - [x] 13.7 Implement MIDI CC mapping
    - Map CC messages to parameters
    - Implement CC learn functionality
    - _Requirements: 13.5_
  
  - [ ]* 13.8 Write property test for MIDI CC control
    - **Property 14: MIDI CC Parameter Control**
    - **Validates: Requirements 13.5**
  
  - [x] 13.9 Implement MIDI pitch bend
    - Apply pitch bend to base pitch
    - Implement pitch bend range control
    - _Requirements: 13.6_
  
  - [ ]* 13.10 Write property test for MIDI pitch bend
    - **Property 15: MIDI Pitch Bend**
    - **Validates: Requirements 13.6**

- [ ] 14. Checkpoint - Ensure MIDI tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 15. Implement audio engine core
  - [x] 15.1 Implement AudioEngine class
    - Integrate voice allocator, effects chain, and parameter manager
    - Implement audio buffer processing
    - Handle sample rate configuration
    - _Requirements: 1.8, 2.10_
  
  - [x] 15.2 Implement sample-accurate parameter updates
    - Process parameter changes within audio buffer
    - Interpolate parameter values if needed
    - _Requirements: 2.10_
  
  - [ ]* 15.3 Write unit test for parameter update latency
    - Measure time between parameter change and audio output change
    - Verify < 10ms latency
    - _Requirements: 2.10_
  
  - [ ]* 15.4 Write unit test for audio processing latency
    - Measure processing time for 512-sample buffer at 48kHz
    - Verify < 10ms processing time
    - _Requirements: 12.1, 12.2_

- [ ] 16. Implement VST3 plugin
  - [ ] 16.1 Implement VST3 plugin class
    - Extend Steinberg::Vst::AudioEffect
    - Implement initialize, process, setState, getState
    - Register as instrument plugin
    - _Requirements: 8.1, 8.2_
  
  - [ ] 16.2 Integrate audio engine with VST3
    - Route VST3 process callback to audio engine
    - Map VST3 parameters to parameter manager
    - Handle MIDI events from ProcessData
    - _Requirements: 8.3, 8.4, 8.5_
  
  - [ ] 16.3 Implement VST3 state serialization
    - Serialize parameter manager to VST3 state
    - Deserialize VST3 state to parameter manager
    - _Requirements: 8.7, 8.8_
  
  - [ ]* 16.4 Write property test for VST3 state round-trip
    - **Property 9: Plugin State Round-Trip**
    - **Validates: Requirements 8.7, 8.8**
  
  - [ ] 16.5 Configure VST3 output formats
    - Support mono and stereo output
    - _Requirements: 8.6_

- [ ] 17. Implement standalone macOS application
  - [ ] 17.1 Set up macOS application structure
    - Create Xcode project or CMake macOS bundle
    - Configure app bundle and Info.plist
    - _Requirements: 9.1_
  
  - [x] 17.2 Implement CoreAudio integration
    - Initialize CoreAudio output
    - Configure audio device and buffer size
    - Implement audio callback routing to audio engine
    - _Requirements: 9.2, 9.4, 9.5_
  
  - [x] 17.3 Implement CoreMIDI integration
    - Enumerate MIDI input devices
    - Register MIDI input callback
    - Route MIDI messages to audio engine
    - _Requirements: 9.3, 9.6_
  
  - [ ] 17.4 Implement application state persistence
    - Save parameter state on application close
    - Restore parameter state on application launch
    - _Requirements: 9.7, 9.8_

- [ ] 18. Implement user interface
  - [ ] 18.1 Design UI layout
    - Create layout for parameter controls
    - Design waveform display area
    - Design preset browser
    - _Requirements: 11.1, 11.10_
  
  - [ ] 18.2 Implement parameter controls
    - Create knobs/sliders for all parameters
    - Implement visual feedback for parameter changes
    - Support mouse and keyboard input
    - _Requirements: 11.1, 11.2, 11.8, 11.9_
  
  - [ ] 18.3 Implement envelope visualizers
    - Display warm-up phase envelope
    - Display transient/decay phase envelope with curve shapes
    - Display pitch envelope with curve shape
    - _Requirements: 11.3, 11.4, 11.5_
  
  - [ ] 18.4 Implement effects parameter displays
    - Display compressor parameters
    - Display reverb parameters
    - _Requirements: 11.6_
  
  - [ ] 18.5 Implement preset browser UI
    - Display current preset name
    - Implement next/previous buttons
    - Implement save/load buttons
    - _Requirements: 11.7_

- [ ] 19. Implement waveform visualization
  - [ ] 19.1 Implement WaveformDisplay class
    - Render waveform from voice parameters
    - Implement zoom control
    - Display time and amplitude axes
    - _Requirements: 7.1, 7.3, 7.4, 7.5_
  
  - [ ] 19.2 Implement waveform update mechanism
    - Update waveform on parameter change (debounced to 30fps)
    - Highlight playback position on MIDI trigger
    - Use background thread for rendering
    - _Requirements: 7.2, 7.6_
  
  - [ ]* 19.3 Write unit test for waveform update rate
    - Verify waveform updates within 33ms
    - _Requirements: 7.2_
  
  - [ ] 19.4 Implement waveform zoom
    - Allow zooming to view specific portions
    - _Requirements: 7.7_

- [ ] 20. Create factory presets
  - [ ] 20.1 Design and create factory presets
    - Create presets covering common kick drum styles (deep sub, punchy, 808, etc.)
    - Save presets to factory preset directory
    - _Requirements: 10.4_

- [ ] 21. Final integration and testing
  - [ ] 21.1 Integration testing
    - Test VST3 plugin in multiple DAWs
    - Test standalone app on macOS
    - Test MIDI device hot-plugging
    - Test audio device changes
    - _Requirements: 8.1-8.8, 9.1-9.8_
  
  - [ ]* 21.2 Performance testing
    - Measure CPU usage with 8 voices at 48kHz
    - Verify < 5% CPU on M1 MacBook Pro
    - _Requirements: 12.5_
  
  - [ ]* 21.3 Run all property tests
    - Execute all 18 property tests with 100+ iterations
    - Verify all properties pass
    - _Requirements: All correctness properties_
  
  - [ ]* 21.4 Run all unit tests
    - Execute complete unit test suite
    - Verify 90%+ code coverage
    - _Requirements: All requirements_

- [ ] 22. Final checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties (minimum 100 iterations each)
- Unit tests validate specific examples and edge cases
- C++ is used for audio engine (industry standard for VST plugins)
- Platform-specific code for macOS (CoreAudio, CoreMIDI)
- VST3 SDK required for plugin development
