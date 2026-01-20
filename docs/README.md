# Documentation

This directory contains all documentation for the Kick Drum Synthesizer project.

## Directory Structure

```
docs/
├── implementation/     # Component implementation documentation
├── checkpoints/        # Development checkpoint summaries
├── BUILDING.md         # Build instructions
├── CONTRIBUTING.md     # Contribution guidelines
├── PROJECT_STRUCTURE.md # Project organization
├── SETUP_COMPLETE.md   # Initial setup documentation
├── STANDALONE_APP_GUIDE.md # Standalone app user guide
├── COREAUDIO_INTEGRATION.md # CoreAudio implementation details
└── COREMIDI_INTEGRATION.md  # CoreMIDI implementation details
```

## Quick Links

### Getting Started

- [Building the Project](BUILDING.md) - How to build the synthesizer
- [Project Structure](PROJECT_STRUCTURE.md) - Overview of the codebase
- [Standalone App Guide](STANDALONE_APP_GUIDE.md) - Using the standalone application

### Implementation Documentation

All component implementation docs are in the `implementation/` directory:

- Audio Engine
- Generators (Sine Driver, Harmonic Membrane, Noise)
- Envelopes (Dual-Phase, Pitch Envelope, Curves)
- Effects (Compressor, Reverb, Effects Chain)
- MIDI (Message Parsing, CC Mapping, Pitch Bend, Pitch Tracking)
- Parameters (Parameter System, Manager)
- Presets (Preset, Preset Manager)
- Voice (Voice, Voice Allocator)
- Master Output & Safety

### Platform Integration

- [CoreAudio Integration](COREAUDIO_INTEGRATION.md) - macOS audio output
- [CoreMIDI Integration](COREMIDI_INTEGRATION.md) - macOS MIDI input

### Development

- [Contributing Guidelines](CONTRIBUTING.md) - How to contribute
- [Checkpoints](checkpoints/) - Development milestone summaries

## Documentation Standards

All implementation documentation follows this structure:

1. **Overview** - What the component does
2. **Requirements** - Which spec requirements it satisfies
3. **Implementation** - Technical details
4. **Testing** - Unit tests and validation
5. **Usage** - Code examples
6. **Status** - Completion status

## Spec Documentation

The formal specification is located in `.kiro/specs/kick-drum-synthesizer/`:

- `requirements.md` - User stories and acceptance criteria
- `design.md` - Architecture and design decisions
- `tasks.md` - Implementation task list

## Additional Resources

- Main README: `../README.md`
- Build Scripts: `../scripts/`
- Source Code: `../src/`
- Tests: `../tests/`
