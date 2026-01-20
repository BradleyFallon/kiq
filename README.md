# Kick Drum Synthesizer

A professional kick drum synthesizer available as both a VST3 plugin and standalone macOS application. Features a three-generator synthesis engine with ring modulation, dual-phase envelope system, and real-time waveform visualization.

## Features

- **Three-Generator Synthesis**: Sine Driver, Harmonic Membrane, and Noise Generator
- **Ring Modulation**: Complex harmonic content generation
- **Dual-Phase Envelope System**: Warm-Up Phase and Transient/Decay Phase
- **Master Effects**: Compressor and Reverb
- **Real-Time Waveform Visualization**: See your sound as you design it
- **Preset Management**: Save and recall your favorite kick drum sounds
- **Low-Latency Audio Processing**: Professional-grade performance
- **VST3 Plugin**: Use in your favorite DAW
- **Standalone Application**: Run without a DAW on macOS

## Requirements

### Build Requirements

- CMake 3.20 or later
- C++17 compatible compiler (Clang, GCC, or MSVC)
- macOS 11.0 (Big Sur) or later (for standalone app)
- VST3 SDK (for VST3 plugin)

### Runtime Requirements

- macOS 11.0 or later
- Audio interface (CoreAudio compatible)
- MIDI controller (optional)

## Quick Start

### Build the Standalone App

```bash
./scripts/build/build_standalone.sh
```

### Run the Standalone App

```bash
./scripts/run_standalone.sh
```

See [docs/STANDALONE_APP_GUIDE.md](docs/STANDALONE_APP_GUIDE.md) for usage instructions.

## Building

See [docs/BUILDING.md](docs/BUILDING.md) for detailed build instructions.

### Quick Build

```bash
# Build everything
./scripts/build/build.sh

# Build standalone app only
./scripts/build/build_standalone.sh
```

## Testing

See [scripts/README.md](scripts/README.md) for all available test scripts.

### Run All Tests

```bash
cd build
ctest
```

### Run Specific Component Tests

```bash
./scripts/test/test_sine_driver_compile.sh
./scripts/test/test_audio_engine_master.sh
```

## Installation

### VST3 Plugin

After building, the VST3 plugin will be installed to:
- macOS: `~/Library/Audio/Plug-Ins/VST3/KickDrumSynth.vst3`

Or manually install:
```bash
cd build
cmake --install .
```

### Standalone Application

After building, the standalone app will be in:
- `build/bin/KickDrumSynthStandalone.app`

Copy to Applications folder:
```bash
cp -r build/bin/KickDrumSynthStandalone.app /Applications/
```

## Usage

### VST3 Plugin

1. Open your DAW (Ableton Live, Logic Pro, etc.)
2. Create a new instrument track
3. Load "Kick Drum Synth" from your plugin list
4. Play MIDI notes to trigger kick drum sounds
5. Adjust parameters to design your sound

### Standalone Application

1. Launch "Kick Drum Synthesizer" from Applications
2. Select your audio output device
3. Select your MIDI input device (optional)
4. Play MIDI notes or click the trigger button
5. Adjust parameters to design your sound

## Parameters

### Generators
- **Base Pitch**: Fundamental frequency (20Hz - 200Hz)
- **Sine Level**: Sine driver output level
- **Harmonic Ratio**: Harmonic frequency ratio (0.5x - 8.0x)
- **Harmonic Level**: Harmonic output level
- **Harmonic Mod Depth**: Ring modulation depth for harmonics
- **Noise Level**: Noise output level
- **Noise Mod Depth**: Ring modulation depth for noise

### Envelopes
- **Warm-Up Duration**: Pre-transient phase duration (0-100ms)
- **Warm-Up Start Freq**: Starting frequency for warm-up sweep
- **Warm-Up Amplitude**: Warm-up phase level
- **Attack**: Attack time (0-1000ms)
- **Decay**: Decay time (0-5000ms)
- **Sustain**: Sustain level (0-100%)
- **Release**: Release time (0-5000ms)
- **Pitch Envelope Depth**: Pitch modulation amount (0-2000Hz)
- **Envelope Curves**: Shape of each envelope segment

### Effects
- **Compressor**: Threshold, Ratio, Attack, Release, Mix
- **Reverb**: Room Size, Decay Time, Damping, Mix

### Master
- **Master Level**: Final output level

## Presets

Presets are stored in JSON format with `.kdpreset` extension.

### Factory Presets Location
- macOS: `~/Library/Application Support/KickDrumSynth/Presets/Factory/`

### User Presets Location
- macOS: `~/Library/Application Support/KickDrumSynth/Presets/User/`

## Documentation

- [Building](docs/BUILDING.md) - Build instructions
- [Standalone App Guide](docs/STANDALONE_APP_GUIDE.md) - Using the standalone app
- [Project Structure](docs/PROJECT_STRUCTURE.md) - Codebase organization
- [Contributing](docs/CONTRIBUTING.md) - Contribution guidelines
- [Implementation Docs](docs/implementation/) - Component documentation
- [Platform Integration](docs/) - CoreAudio and CoreMIDI docs

## Development

### Project Structure

```
kick-drum-synthesizer/
├── src/
│   ├── audio_engine/      # Core DSP and synthesis
│   ├── ui/                # User interface
│   ├── platform/          # Platform-specific code (CoreAudio, CoreMIDI)
│   ├── vst3/              # VST3 plugin wrapper
│   └── standalone/        # Standalone application
├── tests/
│   ├── unit/              # C++ unit tests (Google Test)
│   ├── property/          # Property-based tests (fast-check)
│   └── manual/            # Manual integration tests
├── scripts/
│   ├── build/             # Build scripts
│   ├── test/              # Test scripts
│   └── run_*.sh           # Runtime scripts
├── docs/                  # Documentation
│   ├── implementation/    # Component docs
│   └── checkpoints/       # Development milestones
├── external/              # External dependencies (VST3 SDK)
└── .kiro/specs/           # Formal specification
```

### Contributing

See [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

## License

[Your License Here]

## Credits

- VST is a trademark of Steinberg Media Technologies GmbH
- Built with VST3 SDK
- Uses Google Test for unit testing
- Uses fast-check for property-based testing

## Support

For issues, questions, or feature requests, please open an issue on GitHub.
