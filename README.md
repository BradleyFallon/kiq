# Kiq

Kiq is a small kick-drum synthesizer with a shared C++17 DSP engine, a raw
VST3 wrapper, and a native macOS app. The plugin and standalone targets share
the same VSTGUI editor.

The current synthesis model is a compact, physically informed kick model:

```text
absolute-Hz tension trajectory ───────────┐
strike position ──────────────────────────┤ 3-mode membrane ─┐
dB energy-decay trajectory ───────────────┘                 │
trigger ─ finite-contact impact + band-limited air ─────────┼─ mix ─ output gain ─ soft clip
```

The tension and energy-decay trajectories each have four points and one
curvature value per segment. Tension is expressed as the membrane's
fundamental frequency and interpolated in logarithmic frequency space. Strike
position samples circular-membrane mode shapes from the center toward the rim,
changing both body level and the excitation of two short-lived upper modes.
Every trigger resets the modal phases and deterministic air-noise state, so the
same settings and velocity produce the same samples.

## Current status

- The graphical macOS CoreAudio/CoreMIDI standalone builds and runs.
- The VST3 instrument builds, opens its custom editor, and passes Steinberg's
  47 validator tests.
- All 26 trajectory, membrane, transient, and output parameters are editable
  from the UI and automatable in a VST3 host.
- The editor shows a thin peak-waveform line rendered through the same voice
  and output soft clip used for audio, plus a time-aligned tuning curve.
- HIT audition, a sample-accurate 40–240 BPM LOOP audition mode, and an output
  peak/clip meter work in both targets. LOOP runs from the audio clock but is
  not synchronized to DAW transport.
- Old presets from the previous ADSR/ring-modulation engine are intentionally
  unsupported.

## Build and test

Requirements: CMake 3.25+, Xcode/Apple Clang, and the VST3 SDK (including its
VSTGUI submodule) at `external/vst3sdk` for either graphical target.

Build the standalone and tests with the standard generator:

```bash
cmake -S . -B build -DBUILD_VST3=OFF -DBUILD_STANDALONE=ON -DBUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
open build/bin/KickDrumSynthStandalone.app
```

Build and validate the VST3 using the Xcode generator:

```bash
cmake -G Xcode -S . -B cmake-build-vst3-xcode \
  -DBUILD_VST3=ON -DBUILD_STANDALONE=ON -DBUILD_TESTS=OFF
cmake --build cmake-build-vst3-xcode --config Release --target KickDrumSynth
```

The SDK's post-build step runs its validator and links the result into
`~/Library/Audio/Plug-Ins/VST3/`.

## Editor controls

- Drag a numbered trajectory point vertically for value and horizontally for
  time. Point 1 is fixed at 0 ms.
- Drag the small dot on a segment to change its curvature.
- Drag a knob vertically, use Shift for fine adjustment, use the mouse wheel,
  or double-click to restore its default.
- Click **HIT**, or press Space/Return while the editor has focus, to audition.
- Click **LOOP** to repeat the current hit, then set its 40–240 BPM audition
  tempo with the adjacent knob. The loop is not synchronized to host transport.
- The response preview refreshes after parameter changes. It shows the current
  post-soft-clip peak waveform, time-aligned fundamental tuning, duration, and
  peak level.

## Default hit

| Path | Time | Value |
|---|---:|---:|
| Tension | 0 ms | 220 Hz |
| Tension | 18 ms | 105 Hz |
| Tension | 58 ms | 52 Hz |
| Tension | 200 ms | 52 Hz |
| Energy decay | 0 ms | -60 dB |
| Energy decay | 0.5 ms | 0 dB |
| Energy decay | 65 ms | -8 dB |
| Energy decay | 220 ms | -60 dB |

The transient combines a finite-contact, zero-area impact wavelet with a
band-limited deterministic air burst whose default decay is 7 ms. Beater
hardness controls both contact duration and air bandwidth. The output uses a
continuous, monotonic soft-clip shoulder. MIDI note number does not retune the
trajectory; velocity scales the hit, note-off leaves the one-shot running, and
all-notes-off stops it. Physical parameters are captured at trigger time so
editing cannot zipper a ringing hit; output gain remains live through a short
smooth ramp. A five-millisecond raised-cosine tail prevents a non-silent final
energy point from ending abruptly.

See [the core architecture](docs/implementation/TRAJECTORY_DRIVEN_CORE.md),
[building notes](docs/BUILDING.md), and the
[standalone guide](docs/STANDALONE_APP_GUIDE.md).
