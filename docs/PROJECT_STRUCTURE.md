# Project Structure

This document describes the organization of the Kick Drum Synthesizer codebase.

## Directory Layout

```
kick-drum-synthesizer/
├── .kiro/                          # Kiro specification files
│   └── specs/
│       └── kick-drum-synthesizer/
│           ├── requirements.md     # Requirements specification
│           ├── design.md           # Design document
│           └── tasks.md            # Implementation tasks
│
├── src/                            # Source code
│   ├── audio_engine/               # Core DSP and synthesis engine
│   │   ├── include/                # Public headers
│   │   ├── core/                   # Core engine components
│   │   ├── generators/             # Oscillators and noise generators
│   │   ├── modulation/             # Ring modulation and mixing
│   │   ├── envelopes/              # Envelope system
│   │   ├── effects/                # Compressor and reverb
│   │   ├── voice/                  # Voice management
│   │   ├── parameters/             # Parameter management
│   │   ├── presets/                # Preset management
│   │   ├── midi/                   # MIDI handling
│   │   └── utils/                  # Utilities (DSP, JSON)
│   │
│   ├── ui/                         # User interface
│   │   ├── include/                # Public headers
│   │   ├── waveform/               # Waveform display
│   │   ├── controls/               # UI controls (knobs, sliders)
│   │   ├── visualizers/            # Envelope visualizers
│   │   └── preset_browser/         # Preset browser UI
│   │
│   ├── platform/                   # Platform-specific code
│   │   ├── include/                # Public headers
│   │   ├── audio/                  # Audio interface (CoreAudio)
│   │   ├── midi/                   # MIDI interface (CoreMIDI)
│   │   └── filesystem/             # File system utilities
│   │
│   ├── vst3/                       # VST3 plugin wrapper
│   │   ├── KickSynthVST3.cpp       # Plugin entry point
│   │   ├── KickSynthController.cpp # VST3 controller
│   │   └── KickSynthProcessor.cpp  # VST3 processor
│   │
│   └── standalone/                 # Standalone application
│       ├── main.cpp                # Application entry point
│       ├── StandaloneApp.cpp       # Application class
│       └── Info.plist.in           # macOS bundle info
│
├── tests/                          # Test suite
│   ├── unit/                       # C++ unit tests (Google Test)
│   │   ├── generators/             # Generator tests
│   │   ├── modulation/             # Modulation tests
│   │   ├── envelopes/              # Envelope tests
│   │   ├── effects/                # Effects tests
│   │   ├── voice/                  # Voice management tests
│   │   ├── parameters/             # Parameter tests
│   │   ├── presets/                # Preset tests
│   │   ├── midi/                   # MIDI tests
│   │   ├── core/                   # Core engine tests
│   │   ├── utils/                  # Utility tests
│   │   └── main.cpp                # Test runner
│   │
│   └── property/                   # Property-based tests (fast-check)
│       ├── src/                    # Test source files
│       ├── package.json            # Node.js dependencies
│       ├── jest.config.js          # Jest configuration
│       ├── tsconfig.json           # TypeScript configuration
│       └── README.md               # Property test documentation
│
├── external/                       # External dependencies
│   ├── README.md                   # Dependency documentation
│   └── vst3sdk/                    # VST3 SDK (not included, must download)
│
├── build/                          # Build output (generated)
│   ├── bin/                        # Executables
│   ├── lib/                        # Libraries
│   └── ...
│
├── presets/                        # Preset files
│   ├── factory/                    # Factory presets
│   └── user/                       # User presets (gitignored)
│
├── CMakeLists.txt                  # Root CMake configuration
├── build.sh                        # Build script
├── README.md                       # Project overview
├── BUILDING.md                     # Build instructions
├── CONTRIBUTING.md                 # Contribution guidelines
├── PROJECT_STRUCTURE.md            # This file
├── .gitignore                      # Git ignore rules
└── LICENSE                         # Project license
```

## Component Descriptions

### Audio Engine (`src/audio_engine/`)

The core DSP and synthesis engine, written in C++. This is the heart of the synthesizer.

**Key Components:**
- **Generators**: Sine Driver, Harmonic Membrane, Noise Generator
- **Modulation**: Ring Modulator, Mixer
- **Envelopes**: Dual-Phase Envelope, Pitch Envelope, Curve Functions
- **Effects**: Compressor, Reverb, Effects Chain
- **Voice**: Voice class, Voice Allocator
- **Parameters**: Parameter class, Parameter Manager
- **Presets**: Preset class, Preset Manager
- **MIDI**: MIDI Message, MIDI Handler
- **Core**: Audio Engine, Audio Buffer
- **Utils**: DSP utilities, JSON serialization

### User Interface (`src/ui/`)

The graphical user interface components.

**Key Components:**
- **Waveform Display**: Real-time waveform visualization
- **Controls**: Knobs, sliders, buttons
- **Visualizers**: Envelope visualizers
- **Preset Browser**: Preset selection and management UI

### Platform Layer (`src/platform/`)

Platform-specific code for audio, MIDI, and file system access.

**Key Components:**
- **Audio Interface**: CoreAudio integration (macOS)
- **MIDI Interface**: CoreMIDI integration (macOS)
- **File System**: Preset file management

### VST3 Plugin (`src/vst3/`)

VST3 plugin wrapper that integrates the audio engine with the VST3 SDK.

**Key Components:**
- **VST3 Entry Point**: Plugin registration and initialization
- **Controller**: Parameter management and UI
- **Processor**: Audio processing and MIDI handling

### Standalone Application (`src/standalone/`)

Standalone macOS application that runs without a DAW.

**Key Components:**
- **Main**: Application entry point
- **Standalone App**: Application class integrating audio, MIDI, and UI

### Tests (`tests/`)

Comprehensive test suite with both unit tests and property-based tests.

**Unit Tests (C++):**
- Test specific functionality
- Test edge cases
- Test error handling
- Use Google Test framework

**Property-Based Tests (TypeScript):**
- Test universal properties
- Use fast-check framework
- Minimum 100 iterations per test
- Reference design document properties

## Build System

### CMake Structure

```
CMakeLists.txt                      # Root configuration
├── src/audio_engine/CMakeLists.txt # Audio engine library
├── src/ui/CMakeLists.txt           # UI library
├── src/platform/CMakeLists.txt     # Platform layer library
├── src/vst3/CMakeLists.txt         # VST3 plugin
├── src/standalone/CMakeLists.txt   # Standalone app
└── tests/CMakeLists.txt            # Test suite
    ├── unit/CMakeLists.txt         # C++ unit tests
    └── property/CMakeLists.txt     # Property-based tests
```

### Build Targets

- `kick_drum_audio_engine`: Audio engine library
- `kick_drum_ui`: UI library
- `kick_drum_platform`: Platform layer library
- `KickDrumSynth`: VST3 plugin
- `KickDrumSynthStandalone`: Standalone application
- `kick_drum_tests`: Unit test executable

## Dependencies

### External Dependencies

- **VST3 SDK**: Required for VST3 plugin (not included)
- **Google Test**: Unit testing framework (auto-downloaded)
- **fast-check**: Property-based testing (npm package)

### System Dependencies

- **macOS**: CoreAudio, CoreMIDI, Cocoa, QuartzCore
- **Linux**: ALSA, JACK (optional)
- **Windows**: WASAPI

## File Naming Conventions

- **Headers**: `ClassName.h`
- **Implementation**: `ClassName.cpp`
- **Tests**: `ClassNameTest.cpp`
- **CMake**: `CMakeLists.txt`

## Code Organization Principles

1. **Separation of Concerns**: Audio engine, UI, and platform code are separate
2. **Modularity**: Each component is self-contained
3. **Testability**: All components have corresponding tests
4. **Platform Abstraction**: Platform-specific code is isolated
5. **Clear Dependencies**: Dependencies flow from high-level to low-level

## Adding New Components

### Adding a New Audio Engine Component

1. Create header in `src/audio_engine/include/`
2. Create implementation in appropriate subdirectory
3. Add to `src/audio_engine/CMakeLists.txt`
4. Create unit test in `tests/unit/`
5. Update documentation

### Adding a New UI Component

1. Create header in `src/ui/include/`
2. Create implementation in appropriate subdirectory
3. Add to `src/ui/CMakeLists.txt`
4. Create tests if applicable
5. Update documentation

### Adding a New Test

1. Create test file in appropriate `tests/unit/` subdirectory
2. Add to `tests/unit/CMakeLists.txt`
3. Follow existing test patterns
4. Run tests to verify

## Documentation

- **README.md**: Project overview and quick start
- **BUILDING.md**: Detailed build instructions
- **CONTRIBUTING.md**: Contribution guidelines
- **PROJECT_STRUCTURE.md**: This file
- **Design Document**: `.kiro/specs/kick-drum-synthesizer/design.md`
- **Requirements**: `.kiro/specs/kick-drum-synthesizer/requirements.md`
- **Tasks**: `.kiro/specs/kick-drum-synthesizer/tasks.md`

## Version Control

### Ignored Files

- Build artifacts (`build/`, `bin/`, `lib/`)
- IDE files (`.vscode/`, `.idea/`)
- User presets (`presets/user/`)
- Node modules (`tests/property/node_modules/`)
- VST3 SDK (`external/vst3sdk/`)

### Tracked Files

- Source code (`src/`)
- Tests (`tests/`)
- Build configuration (`CMakeLists.txt`, `*.cmake`)
- Documentation (`*.md`)
- Factory presets (`presets/factory/`)

## Future Expansion

Areas for potential expansion:

- **Additional Platforms**: Linux, Windows support
- **Additional Formats**: AU, AAX plugin formats
- **Additional Features**: More generators, effects, modulation
- **Additional Tests**: Integration tests, performance tests
- **Additional Documentation**: API documentation, user manual
