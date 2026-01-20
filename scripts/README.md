# Build and Test Scripts

This directory contains all build and test scripts for the Kick Drum Synthesizer project.

## Directory Structure

```
scripts/
├── build/          # Build scripts
├── test/           # Test scripts
└── run_*.sh        # Runtime scripts
```

## Build Scripts

### `build/build.sh`
Main build script for the entire project.

```bash
./scripts/build/build.sh
```

### `build/build_standalone.sh`
Build only the standalone macOS application.

```bash
./scripts/build/build_standalone.sh
```

## Runtime Scripts

### `run_standalone.sh`
Run the standalone application.

```bash
./scripts/run_standalone.sh
```

### `run_all_generator_tests.sh`
Run all generator unit tests.

```bash
./scripts/run_all_generator_tests.sh
```

### `run_checkpoint_10_tests.sh`
Run checkpoint 10 tests.

```bash
./scripts/run_checkpoint_10_tests.sh
```

### `run_voice_tests.sh`
Run voice-related tests.

```bash
./scripts/run_voice_tests.sh
```

## Test Scripts

All test scripts are in the `test/` subdirectory. They compile and run specific component tests.

### Component Tests

- `test_sine_driver_compile.sh` - Sine oscillator tests
- `test_harmonic_membrane_compile.sh` - Harmonic oscillator tests
- `test_noise_generator` - Noise generator tests
- `test_ring_modulator_compile.sh` - Ring modulator tests
- `test_generator_mixer_compile.sh` - Generator mixer tests

### Envelope Tests

- `test_envelope_curves_compile.sh` - Envelope curve tests
- `test_dual_phase_envelope_compile.sh` - Dual-phase envelope tests
- `test_pitch_envelope_compile.sh` - Pitch envelope tests

### Effects Tests

- `test_compressor_compile.sh` - Compressor tests
- `test_reverb_compile.sh` - Reverb tests
- `test_effects_chain_compile.sh` - Effects chain tests

### MIDI Tests

- `test_midi_message.sh` - MIDI message parsing tests
- `test_midi_cc_compile.sh` - MIDI CC mapping tests
- `test_pitch_bend.sh` - Pitch bend tests
- `test_pitch_tracking_compile.sh` - Pitch tracking tests

### Parameter Tests

- `test_parameter_compile.sh` - Parameter class tests
- `test_parameter_manager_compile.sh` - Parameter manager tests
- `test_json_serialization_compile.sh` - JSON serialization tests
- `test_sample_accurate_params.sh` - Sample-accurate parameter tests
- `test_sample_accurate_simple.sh` - Simple sample-accurate tests

### Preset Tests

- `test_preset_compile.sh` - Preset class tests
- `test_preset_manager_compile.sh` - Preset manager tests

### Voice Tests

- `test_voice_compile.sh` - Voice class tests
- `test_voice_allocator_compile.sh` - Voice allocator tests

### Audio Engine Tests

- `test_audio_engine_master.sh` - Audio engine integration tests
- `test_master_output_simple.sh` - Master output tests
- `test_dsp_utils.sh` - DSP utilities tests
- `test_dsp_utils_compile.sh` - DSP utilities compilation tests

### Platform Tests

- `test_coreaudio_compile.sh` - CoreAudio integration tests
- `test_coremidi_compile.sh` - CoreMIDI integration tests

### Standalone App Tests

- `test_standalone_auto.sh` - Automated standalone app test
- `test_standalone_quick.sh` - Quick standalone app test

## Usage

Most test scripts can be run directly:

```bash
./scripts/test/test_sine_driver_compile.sh
```

Or run all tests using CMake:

```bash
cd build
ctest
```

## Notes

- All scripts should be run from the project root directory
- Build scripts create artifacts in the `build/` directory
- Test scripts compile and run tests, showing results in the terminal
