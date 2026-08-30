# Kiq

Kiq is a small kick-drum synthesizer with a shared C++17 DSP engine, a raw
VST3 wrapper, and a terminal-driven macOS standalone app.

The current synthesis model is deliberately simple:

```text
absolute-Hz pitch trajectory ─┐
dB amplitude trajectory ─ sine body ─┐
                                   mix ─ output gain
trigger ─ click + filtered noise ────┘
```

Each pitch and amplitude trajectory has four fixed points and one curvature
value per segment. Pitch is interpolated in logarithmic frequency space.
Every trigger resets oscillator phase and deterministic noise state, so the
same settings and velocity produce the same samples.

## Current status

- macOS CoreAudio/CoreMIDI standalone builds and runs.
- The VST3 instrument builds and passes Steinberg's validator.
- The plugin exposes all 26 trajectory, transient, phase, and output parameters.
- There is no custom graphical editor yet.
- Old presets from the previous ADSR/ring-modulation engine are intentionally
  unsupported.

## Build and test

Requirements: CMake 3.20+, Xcode/Apple Clang, and the VST3 SDK at
`external/vst3sdk` for plugin builds.

Build the standalone and tests with the standard generator:

```bash
cmake -S . -B build -DBUILD_VST3=OFF -DBUILD_STANDALONE=ON -DBUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Build and validate the VST3 using the Xcode generator:

```bash
cmake -G Xcode -S . -B cmake-build-vst3-xcode \
  -DBUILD_VST3=ON -DBUILD_STANDALONE=ON -DBUILD_TESTS=OFF
cmake --build cmake-build-vst3-xcode --config Release --target KickDrumSynth
```

The SDK's post-build step runs its validator and links the result into
`~/Library/Audio/Plug-Ins/VST3/`.

## Default hit

| Path | Time | Value |
|---|---:|---:|
| Pitch | 0 ms | 220 Hz |
| Pitch | 18 ms | 105 Hz |
| Pitch | 58 ms | 52 Hz |
| Pitch | 200 ms | 52 Hz |
| Amplitude | 0 ms | -60 dB |
| Amplitude | 0.5 ms | 0 dB |
| Amplitude | 65 ms | -8 dB |
| Amplitude | 220 ms | -60 dB |

The transient adds a modest two-sample click and filtered deterministic noise
with a 7 ms decay. MIDI note number does not retune the trajectory; velocity
scales the hit, note-off leaves the one-shot running, and all-notes-off stops it.

See [the core architecture](docs/implementation/TRAJECTORY_DRIVEN_CORE.md),
[building notes](docs/BUILDING.md), and the
[standalone guide](docs/STANDALONE_APP_GUIDE.md).
