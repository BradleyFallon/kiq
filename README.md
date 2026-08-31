# Kiq

Kiq is a small kick-drum synthesizer with a shared C++17 DSP engine, a raw
VST3 wrapper, and a native macOS app. The plugin and standalone targets share
the same VSTGUI editor.

The current synthesis model is a compact, physically informed kick model:

```text
absolute-Hz tension + dB energy + strike + phase ── 3-mode membrane ─┐
trigger + beater controls ───────────────────────── impact + air ───┼─ mix
imported/preset audio ───────────────────────── optional sample ────┘

mix ─ output gain ─ 3-band EQ ─ saturation ─ ceiling soft limiter
```

The tension and energy-decay trajectories each have four points and one
curvature value per segment. Tension is expressed as the membrane's
fundamental frequency and interpolated in logarithmic frequency space. Strike
position samples circular-membrane mode shapes from the center toward the rim,
changing both body level and the excitation of two short-lived upper modes.
The membrane, finite-contact impact, band-limited air burst, and optional
resampled transient are independently levelled layers. Fundamental phase can
be rotated directly or locked to a chosen point on the pitch trajectory.
Every trigger resets the model and deterministic air-noise state, so the same
settings, sample layer, and velocity produce the same samples.

## Current status

- The graphical macOS CoreAudio/CoreMIDI standalone builds and runs.
- The VST3 instrument builds, opens its custom editor, and passes Steinberg's
  47 validator tests.
- All 35 trajectory, layer, phase, and output parameters are editable from the
  UI and automatable in a VST3 host. VST3 note and automation points retain
  their sample offsets, with parameter changes applied before notes at the same
  offset.
- The editor shows a thin waveform line rendered through the complete audio
  engine, plus a time-aligned tuning curve. Imported reference waveform and
  pitch estimates are overlaid for comparison.
- A dropped or selected WAV is downmixed and analyzed for a likely kick,
  auto-fits the physical model, and supplies an optional extracted transient
  sample. **FIT** reapplies the model estimate and **ALIGN** applies its phase
  estimate.
- Seven factory presets, strict current-format `.kiqpreset` save/load with an
  optional embedded sample, and bounded parameter undo/redo are available in
  both editors.
- Clicking **EXPORT / DRAG** saves the current hit as deterministic 48 kHz,
  mono 24-bit PCM WAV; dragging it offers the same rendered file to hosts that
  accept file drops.
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
- Use the **MODEL** page for membrane, impact, air, sample, strike, phase, air
  decay, and beater controls. Use **OUTPUT** for low/mid/high EQ, saturation,
  limiter ceiling, and final output gain.
- Toggle **PHASE LOCK** above the tension graph, then drag its vertical marker
  to choose when the fundamental must reach the phase set by the **PHASE**
  knob.
- Open **PRESET** for the factory set or `.kiqpreset` save/load. The arrow
  buttons undo and redo parameter snapshots; Command-Z and
  Shift-Command-Z work on macOS.
- Click **IMPORT**, or drop a WAV anywhere on the editor, to analyze and match
  a reference. The import also extracts a short transient into the optional
  sample layer when possible. **FIT** and **ALIGN** can be reapplied after
  further edits.
- Click **HIT**, or press Space/Return while the editor has focus, to audition.
- Click **LOOP** to repeat the current hit, then set its 40–240 BPM audition
  tempo with the adjacent knob. The loop is not synchronized to host transport.
- The response preview refreshes after parameter changes. It shows the current
  post-output-stage waveform, time-aligned fundamental tuning, duration, peak
  level, and any imported reference overlays.
- Click **EXPORT / DRAG** to choose a WAV destination, or drag the button into
  a DAW/file target that accepts file-path drops.

Current limitations are deliberate and small-project oriented: reference
matching is a deterministic heuristic rather than an exact inversion of the
source; the imported sample is a short extracted transient, not full reference
playback; imports are capped at 128 MiB encoded/12 million decoded frames; and
editor undo tracks the 35 parameters, not sample replacement.
Loading a preset file or reference therefore starts a fresh undo history;
factory-preset changes remain undoable. MIDI note
number does not transpose a hit, pitch bend and MIDI CC are not connected, and
LOOP is not synchronized to host transport. The output limiter is a smooth
ceiling-scaled clipper, not a look-ahead dynamics processor.

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
hardness controls both contact duration and air bandwidth. The optional sample
layer is linearly resampled at its source rate and faded at its end. After the
four layers mix, the engine applies output gain, fixed-split low/mid/high EQ,
tanh saturation, and the ceiling-scaled soft limiter. MIDI note number does not
retune the trajectory; velocity scales the hit, note-off leaves the one-shot
running, and all-notes-off stops it. Physical parameters and sample choice are
captured at trigger time so editing cannot zipper a ringing hit; output gain
and the global output stage remain live through short smoothing ramps. A
five-millisecond raised-cosine tail prevents a non-silent final energy point
from ending abruptly.

See [the core architecture](docs/implementation/TRAJECTORY_DRIVEN_CORE.md),
[building notes](docs/BUILDING.md), and the
[standalone guide](docs/STANDALONE_APP_GUIDE.md).
