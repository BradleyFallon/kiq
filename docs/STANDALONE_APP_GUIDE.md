# Kick Drum Synthesizer - Standalone App Guide

## Quick Start

### Building

```bash
./build_standalone.sh
```

### Running

```bash
./run_standalone.sh
```

Or directly:
```bash
./build/bin/KickDrumSynthStandalone.app/Contents/MacOS/KickDrumSynthStandalone
```

## Features

The standalone app provides a terminal-based interface for testing the kick drum synthesizer with:

- **Audio Output**: CoreAudio integration for real-time audio playback
- **MIDI Input**: CoreMIDI integration for MIDI controller support
- **Parameter Control**: Adjust synthesis parameters in real-time
- **Note Triggering**: Play notes via keyboard or MIDI controller

## Commands

### Playing Notes

```
play <note> [velocity]
```

Examples:
- `play 60` - Play middle C with default velocity (0.8)
- `play 60 1.0` - Play middle C with maximum velocity
- `play 48 0.5` - Play a lower note with softer velocity

Shorthand: `p 60 0.8`

### Stopping Notes

```
stop <note>
```

Example:
- `stop 60` - Release note 60

Shorthand: `s 60`

### Parameter Control

```
param <name> <value>
```

Available parameters:
- `basePitch` (20-200 Hz) - Fundamental frequency
- `sineLevel` (0-1) - Sine driver level
- `harmonicRatio` (0.5-8) - Harmonic frequency ratio
- `harmonicLevel` (0-1) - Harmonic level
- `harmonicModDepth` (0-1) - Harmonic ring modulation depth
- `noiseLevel` (0-1) - Noise level
- `noiseModDepth` (0-1) - Noise ring modulation depth
- `attack` (0-1 seconds) - Attack time
- `decay` (0-5 seconds) - Decay time
- `sustain` (0-1) - Sustain level
- `release` (0-5 seconds) - Release time
- `pitchEnvelopeDepth` (0-2000 Hz) - Pitch envelope depth
- `masterLevel` (0-1) - Master output level

Examples:
- `param basePitch 50.0` - Set base pitch to 50 Hz (deep kick)
- `param attack 0.001` - Set fast attack (1ms)
- `param decay 0.5` - Set 500ms decay
- `param harmonicRatio 2.0` - Set harmonic to 2x base frequency

### List Parameters

```
list
```

Shows all available parameters with their ranges.

Shorthand: `l`

### MIDI Devices

```
midi
```

Lists all available MIDI input devices and shows which one is connected.

Shorthand: `m`

### Help

```
help
```

Shows all available commands.

Shorthand: `h` or `?`

### Quit

```
quit
```

Exit the application.

Shorthand: `q` or `exit`

## MIDI Controller Support

The app automatically connects to the first available MIDI device on startup. You can:

1. **Play notes** - MIDI note-on/off messages trigger synthesis
2. **Control parameters** - MIDI CC messages can be mapped to parameters
3. **Pitch bend** - Pitch bend wheel modulates the pitch

To see available MIDI devices, use the `midi` command.

## Example Session

```
=== Kick Drum Synthesizer ===
Initializing...

Audio initialized:
  Device: Built-in Output
  Sample Rate: 48000 Hz
  Buffer Size: 512 frames

Available MIDI devices:
  [0] USB MIDI Interface (Manufacturer) - Online
Connected to MIDI device: USB MIDI Interface

Audio started successfully

Commands:
  play <note> [velocity]  - Trigger a note (note: 0-127, velocity: 0.0-1.0)
  stop <note>             - Release a note
  param <name> <value>    - Set a parameter
  list                    - List all parameters
  preset <name>           - Load a preset
  save <name>             - Save current settings as preset
  midi                    - List MIDI devices
  help                    - Show this help
  quit                    - Exit application

Example: play 60 0.8
Example: param basePitch 50.0

> play 60 0.8
Playing note 60 with velocity 0.8

> param basePitch 40.0
Set basePitch = 40

> param decay 1.0
Set decay = 1

> play 60 1.0
Playing note 60 with velocity 1

> quit
```

## Sound Design Tips

### Deep Sub Kick
```
param basePitch 35.0
param sineLevel 1.0
param harmonicLevel 0.0
param noiseLevel 0.0
param attack 0.001
param decay 1.5
param pitchEnvelopeDepth 800.0
play 60 1.0
```

### Punchy Kick
```
param basePitch 60.0
param sineLevel 0.8
param harmonicLevel 0.3
param harmonicRatio 2.0
param noiseLevel 0.2
param attack 0.001
param decay 0.3
param pitchEnvelopeDepth 1200.0
play 60 0.9
```

### 808-Style Kick
```
param basePitch 50.0
param sineLevel 0.9
param harmonicLevel 0.1
param harmonicRatio 1.5
param noiseLevel 0.05
param attack 0.001
param decay 0.8
param pitchEnvelopeDepth 1000.0
play 60 0.85
```

## Troubleshooting

### No Audio Output

1. Check that your audio device is working
2. Try adjusting the master level: `param masterLevel 0.8`
3. Make sure you're triggering notes: `play 60 0.8`

### No MIDI Input

1. Check that your MIDI device is connected
2. Use `midi` command to list available devices
3. The app connects to the first device automatically on startup

### Audio Glitches

1. Try increasing the buffer size (requires rebuild with different CoreAudio settings)
2. Close other audio applications
3. Check CPU usage

## Next Steps

- Load and save presets (coming soon)
- GUI interface (coming soon)
- VST3 plugin version (coming soon)

## Requirements

- macOS 11.0 (Big Sur) or later
- Audio output device
- Optional: MIDI input device for controller support

