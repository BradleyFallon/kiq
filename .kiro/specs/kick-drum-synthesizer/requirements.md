# Requirements Document

## Introduction

This document specifies the requirements for a kick drum synthesizer available as both a VST plugin and standalone desktop application for macOS. The system uses subtractive synthesis combining sine wave oscillators and noise generators with flexible envelope shaping and real-time waveform visualization.

## Glossary

- **Kick_Synth**: The complete kick drum synthesis system
- **Audio_Engine**: The DSP processing core that generates audio output
- **Sine_Driver**: The main sine oscillator providing the fundamental tone and transient source
- **Harmonic_Membrane**: Harmonic oscillator adding character, tuning, and tonal color
- **Noise_Generator**: White noise generator for filling out the transient texture
- **Ring_Modulator**: Multiplies two signals for harmonic generation
- **Modulation_Depth**: The amount of ring modulation applied to a signal
- **Warm_Up_Phase**: The pre-transient phase building speaker momentum
- **Transient_Decay_Phase**: The main kick drum attack and decay phase
- **Compressor**: Dynamic range processor for controlling output dynamics
- **Reverb**: Spatial effect processor for adding ambience
- **Envelope**: A time-varying control signal shaping amplitude or pitch
- **Envelope_Curve**: The shape characteristic of an envelope (linear, exponential, logarithmic, custom)
- **VST_Plugin**: The VST3 format plugin version of the synthesizer
- **Standalone_App**: The desktop application version for macOS
- **Parameter**: A user-controllable synthesis parameter
- **Preset**: A saved collection of parameter values
- **Host_DAW**: The digital audio workstation hosting the VST plugin
- **Audio_Buffer**: A block of audio samples processed by the engine
- **MIDI_Note**: A MIDI note-on message triggering synthesis
- **Waveform_Display**: Visual representation of the generated kick drum waveform

## Requirements

### Requirement 1: Three-Generator Synthesis Engine with Ring Modulation

**User Story:** As a sound designer, I want a synthesis engine with three generators (sine driver, harmonic membrane, noise) and ring modulation, so that I can create kick drums with rich harmonic content and complex timbres.

#### Acceptance Criteria

1. WHEN a MIDI note is received, THE Audio_Engine SHALL synthesize a kick drum sound using a Sine_Driver, Harmonic_Membrane, and Noise_Generator
2. THE Sine_Driver SHALL serve as the main tone and transient source
3. THE Harmonic_Membrane SHALL add character, tuning, and tonal color
4. THE Noise_Generator SHALL fill out the transient texture
5. THE Audio_Engine SHALL provide a Ring_Modulator that multiplies the Sine_Driver output with the Harmonic_Membrane output
6. THE Audio_Engine SHALL provide a Ring_Modulator that multiplies the Sine_Driver output with the Noise_Generator output
7. THE Audio_Engine SHALL mix the direct Sine_Driver, ring-modulated harmonics, and ring-modulated noise with independent level controls
8. THE Audio_Engine SHALL generate audio at the host sample rate (44.1kHz, 48kHz, 88.2kHz, 96kHz, or 192kHz)
9. WHEN synthesis parameters are modified, THE Audio_Engine SHALL update the output in real-time without clicks or discontinuities

### Requirement 2: Dual-Phase Envelope System

**User Story:** As a musician, I want a dual-phase envelope system (warm-up and transient/decay), so that I can precisely control both the pre-transient speaker momentum and the main kick drum character.

#### Acceptance Criteria

1. THE Kick_Synth SHALL provide a Warm_Up_Phase envelope controlling the pre-transient sweep
2. THE Kick_Synth SHALL provide a Transient_Decay_Phase envelope controlling the main kick drum attack and decay
3. WHEN configuring the Warm_Up_Phase, THE Kick_Synth SHALL allow adjustment of duration (0ms to 100ms)
4. WHEN configuring the Warm_Up_Phase, THE Kick_Synth SHALL allow adjustment of frequency sweep range (5Hz to 50Hz start frequency)
5. WHEN configuring the Warm_Up_Phase, THE Kick_Synth SHALL allow adjustment of amplitude level
6. WHEN configuring the Transient_Decay_Phase, THE Kick_Synth SHALL allow adjustment of attack, decay, sustain, and release times
7. WHEN configuring the Transient_Decay_Phase, THE Kick_Synth SHALL allow adjustment of pitch envelope depth (0Hz to 2000Hz range)
8. WHEN configuring any envelope, THE Kick_Synth SHALL allow selection of curve shape (linear, exponential, logarithmic, custom)
9. THE Kick_Synth SHALL allow independent curve shaping for each envelope segment
10. WHEN any envelope parameter changes, THE Audio_Engine SHALL update synthesis within 10 milliseconds
11. THE Audio_Engine SHALL ensure phase continuity between the Warm_Up_Phase and Transient_Decay_Phase

### Requirement 3: Ring Modulation and Generator Control

**User Story:** As a sound designer, I want to control ring modulation depth and generator levels, so that I can shape the harmonic content and texture of the kick drum.

#### Acceptance Criteria

1. THE Kick_Synth SHALL provide a harmonic frequency ratio parameter (0.5x to 8.0x relative to Sine_Driver)
2. THE Kick_Synth SHALL provide a harmonic modulation depth parameter controlling ring modulation amount (0% to 100%)
3. THE Kick_Synth SHALL provide a noise modulation depth parameter controlling ring modulation amount (0% to 100%)
4. WHEN harmonic modulation depth is 0%, THE Audio_Engine SHALL output the Harmonic_Membrane without ring modulation
5. WHEN harmonic modulation depth is 100%, THE Audio_Engine SHALL output fully ring-modulated harmonics (Sine_Driver × Harmonic_Membrane)
6. WHEN noise modulation depth is 0%, THE Audio_Engine SHALL output the Noise_Generator without ring modulation
7. WHEN noise modulation depth is 100%, THE Audio_Engine SHALL output fully ring-modulated noise (Sine_Driver × Noise_Generator)
8. THE Kick_Synth SHALL allow independent level control for Sine_Driver, modulated harmonics, and modulated noise

### Requirement 4: Core Synthesis Parameters

**User Story:** As a sound designer, I want direct control over fundamental synthesis parameters, so that I can shape the character of the kick drum.

#### Acceptance Criteria

1. THE Kick_Synth SHALL provide a base pitch parameter controlling the Sine_Driver fundamental frequency (20Hz to 200Hz)
2. THE Kick_Synth SHALL provide a sine driver level parameter controlling the Sine_Driver output amplitude (0% to 100%)
3. THE Kick_Synth SHALL provide a harmonic level parameter controlling the ring-modulated harmonic output amplitude (0% to 100%)
4. THE Kick_Synth SHALL provide a noise level parameter controlling the ring-modulated noise output amplitude (0% to 100%)
5. WHEN MIDI velocity is received, THE Audio_Engine SHALL scale the amplitude envelope depth proportionally
6. THE Kick_Synth SHALL provide a pitch tracking parameter allowing MIDI note number to affect base pitch

### Requirement 5: Master Effects Processing

**User Story:** As a sound designer, I want master effects (compressor and reverb) on the output, so that I can add dynamics control and spatial character to the final kick drum sound.

#### Acceptance Criteria

1. THE Audio_Engine SHALL apply a Compressor to the mixed output before final output
2. THE Audio_Engine SHALL apply a Reverb to the mixed output after compression
3. THE Compressor SHALL provide threshold, ratio, attack, and release parameters
4. THE Compressor SHALL provide a mix/dry-wet parameter (0% to 100%)
5. THE Reverb SHALL provide room size, decay time, and damping parameters
6. THE Reverb SHALL provide a mix/dry-wet parameter (0% to 100%)
7. WHEN compressor or reverb parameters change, THE Audio_Engine SHALL update processing within 10 milliseconds
8. THE Kick_Synth SHALL allow bypassing of the Compressor independently
9. THE Kick_Synth SHALL allow bypassing of the Reverb independently

### Requirement 6: Master Output Control

**User Story:** As a user, I want master output level control, so that I can set the final output volume.

#### Acceptance Criteria

1. THE Kick_Synth SHALL provide a master output level parameter (0% to 100%)
2. THE Audio_Engine SHALL apply the master output level after all effects processing
3. WHEN audio output exceeds 0dBFS, THE Audio_Engine SHALL apply soft clipping to prevent hard clipping

### Requirement 7: Real-Time Waveform Visualization

**User Story:** As a sound designer, I want to see the waveform shape in real-time, so that I can visually understand how parameter changes affect the output.

#### Acceptance Criteria

1. THE Waveform_Display SHALL show the complete kick drum waveform from trigger to end
2. WHEN any synthesis parameter changes, THE Waveform_Display SHALL update within 33 milliseconds (30fps minimum)
3. THE Waveform_Display SHALL show the combined output of all synthesis components including effects (sine driver + harmonics + noise + warm-up + compressor + reverb)
4. THE Waveform_Display SHALL provide time axis markings showing duration in milliseconds
5. THE Waveform_Display SHALL provide amplitude axis markings showing level in dB or percentage
6. WHEN a MIDI note is triggered, THE Waveform_Display SHALL highlight or animate to show the current playback position
7. THE Waveform_Display SHALL allow zooming to view specific portions of the waveform in detail

### Requirement 8: VST Plugin Format

**User Story:** As a music producer, I want to use the kick synth as a VST plugin in my DAW, so that I can integrate it into my production workflow.

#### Acceptance Criteria

1. THE VST_Plugin SHALL implement the VST3 plugin standard
2. WHEN loaded in a Host_DAW, THE VST_Plugin SHALL register as an instrument plugin
3. WHEN the Host_DAW sends MIDI data, THE VST_Plugin SHALL process MIDI_Note messages
4. THE VST_Plugin SHALL expose all synthesis parameters as automatable plugin parameters
5. WHEN the Host_DAW requests audio, THE VST_Plugin SHALL fill the Audio_Buffer with synthesized audio
6. THE VST_Plugin SHALL support both mono and stereo output configurations
7. WHEN the plugin state is saved, THE VST_Plugin SHALL serialize all parameter values
8. WHEN the plugin state is loaded, THE VST_Plugin SHALL restore all parameter values

### Requirement 9: Standalone Desktop Application

**User Story:** As a performer, I want to use the kick synth as a standalone app, so that I can use it without a DAW during live performances.

#### Acceptance Criteria

1. THE Standalone_App SHALL run natively on macOS 11.0 (Big Sur) and later
2. WHEN launched, THE Standalone_App SHALL initialize the Audio_Engine and present the user interface
3. THE Standalone_App SHALL receive MIDI input from connected MIDI devices
4. THE Standalone_App SHALL output audio to the selected system audio device
5. WHEN the user selects an audio device, THE Standalone_App SHALL configure the Audio_Engine for that device's sample rate and buffer size
6. THE Standalone_App SHALL allow the user to configure MIDI input routing
7. WHEN the application is closed, THE Standalone_App SHALL save the current parameter state
8. WHEN the application is launched, THE Standalone_App SHALL restore the previous parameter state

### Requirement 10: Preset Management

**User Story:** As a sound designer, I want to save and recall presets, so that I can organize and reuse my kick drum sounds.

#### Acceptance Criteria

1. WHEN the user saves a preset, THE Kick_Synth SHALL store all current parameter values to a Preset file
2. WHEN the user loads a preset, THE Kick_Synth SHALL restore all parameter values from the Preset file
3. THE Kick_Synth SHALL support preset browsing with next/previous navigation
4. THE Kick_Synth SHALL include a library of factory presets covering common kick drum styles
5. WHERE the VST_Plugin is used, THE Kick_Synth SHALL store presets in the plugin state
6. WHERE the Standalone_App is used, THE Kick_Synth SHALL store presets in the user's documents folder

### Requirement 11: User Interface

**User Story:** As a user, I want a clear and responsive interface, so that I can efficiently design kick drum sounds.

#### Acceptance Criteria

1. THE Kick_Synth SHALL display all synthesis parameters with visual feedback
2. WHEN a parameter is adjusted, THE Kick_Synth SHALL update the visual display within 16 milliseconds (60fps)
3. THE Kick_Synth SHALL provide visual representation of the warm-up phase envelope
4. THE Kick_Synth SHALL provide visual representation of the transient/decay phase envelope with curve shapes
5. THE Kick_Synth SHALL provide visual representation of the pitch envelope with curve shape
6. THE Kick_Synth SHALL display compressor and reverb parameter values
7. THE Kick_Synth SHALL display the current preset name
8. WHEN the user interacts with controls, THE Kick_Synth SHALL provide immediate visual feedback
9. THE Kick_Synth SHALL support both mouse and keyboard input for parameter adjustment
10. THE Kick_Synth SHALL include the Waveform_Display as a prominent visual element

### Requirement 12: Audio Performance

**User Story:** As a music producer, I want low-latency audio processing, so that the synth responds immediately to MIDI input.

#### Acceptance Criteria

1. THE Audio_Engine SHALL process audio buffers with latency not exceeding the buffer size
2. WHEN processing a 512-sample buffer at 48kHz, THE Audio_Engine SHALL complete processing within 10 milliseconds
3. THE Audio_Engine SHALL generate audio without clicks, pops, or discontinuities
4. WHEN multiple notes are triggered rapidly, THE Audio_Engine SHALL handle polyphony up to 8 simultaneous voices
5. THE Audio_Engine SHALL maintain CPU usage below 5% on a 2020 M1 MacBook Pro at 48kHz with 512-sample buffers

### Requirement 13: MIDI Implementation

**User Story:** As a performer, I want standard MIDI control, so that I can play the synth with any MIDI controller.

#### Acceptance Criteria

1. WHEN a MIDI note-on message is received, THE Kick_Synth SHALL trigger synthesis with velocity affecting amplitude
2. WHEN a MIDI note-off message is received, THE Kick_Synth SHALL allow the amplitude envelope to complete naturally
3. THE Kick_Synth SHALL respond to MIDI notes across the full range (0-127)
4. WHEN different MIDI note numbers are received, THE Kick_Synth SHALL adjust the base pitch accordingly
5. THE Kick_Synth SHALL support MIDI CC messages for parameter control
6. THE Kick_Synth SHALL respond to MIDI pitch bend messages

### Requirement 14: File Format and Persistence

**User Story:** As a user, I want my settings and presets saved reliably, so that I don't lose my work.

#### Acceptance Criteria

1. WHEN saving presets, THE Kick_Synth SHALL encode parameter data using JSON format
2. WHEN loading presets, THE Kick_Synth SHALL validate the JSON structure before applying parameters
3. IF a preset file is corrupted, THEN THE Kick_Synth SHALL display an error message and maintain current state
4. THE Kick_Synth SHALL include version information in preset files for future compatibility
5. WHEN preset files are saved, THE Kick_Synth SHALL use the .kdpreset file extension

### Requirement 15: Error Handling and Stability

**User Story:** As a user, I want the synth to handle errors gracefully, so that it doesn't crash or produce dangerous audio levels.

#### Acceptance Criteria

1. IF audio device initialization fails, THEN THE Kick_Synth SHALL display an error message and allow device reselection
2. IF MIDI device connection is lost, THEN THE Kick_Synth SHALL continue operating and attempt reconnection
3. WHEN audio output exceeds 0dBFS, THE Audio_Engine SHALL apply soft clipping to prevent hard clipping
4. IF the Audio_Engine produces invalid values (NaN, infinity), THEN THE Audio_Engine SHALL reset the synthesis state and log the error
5. WHEN the Host_DAW changes sample rate, THE VST_Plugin SHALL reinitialize the Audio_Engine without crashing
6. IF preset loading fails, THEN THE Kick_Synth SHALL maintain the current parameter state and notify the user
