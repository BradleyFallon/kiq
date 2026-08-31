# Standalone app

Build and open the graphical macOS app:

```bash
cmake -S . -B build \
  -DBUILD_VST3=OFF -DBUILD_STANDALONE=ON -DBUILD_TESTS=OFF
cmake --build build --target KickDrumSynthStandalone -j
open build/bin/KickDrumSynthStandalone.app
```

The app opens the default CoreAudio output and connects the first available
MIDI input. Click **HIT** (or press Space/Return after clicking the editor) to
audition the current kick.

The membrane-tension and energy-decay panels are the actual synthesis
trajectories:

- Drag numbered points to edit their value and, except for point 1, time.
- Drag the smaller dot on each segment to edit curvature.
- Drag knobs vertically. Shift-drag is finer, the mouse wheel makes small
  changes, and double-click restores a default.
- Use the **MODEL** page to balance the explicit **MEMBRANE**, **IMPACT**,
  **AIR**, and optional **SAMPLE** layers. **STRIKE** moves from a center hit
  toward the rim; **AIR DECAY** and **BEATER** shape the finite-contact and
  band-limited transient.
- Rotate the fundamental with **PHASE**. Enable **PHASE LOCK** above the tension
  graph and drag its vertical marker to choose the time at which the
  fundamental must reach that phase.
- Use the **OUTPUT** page for fixed-split low, mid, and high EQ, tanh
  saturation, the soft-limiter ceiling, and final output gain.
- The response panel re-renders the complete engine after parameter changes.
  Its thin cyan line is the post-output-stage waveform and the orange line is
  the time-aligned fundamental tuning curve. An imported reference adds purple
  waveform and pitch overlays.
- Click **LOOP** for repeated audition, then set 40–240 BPM with **TEMPO**. The
  audio engine schedules repeats at exact sample offsets, but the loop is not
  synchronized to DAW transport.
- The right-side LEDs show output peak and clip activity.

The header provides the file and matching workflows:

- Open **PRESET** for seven factory sounds or to save/load the strict current
  `.kiqpreset` format. A saved preset includes all 35 parameters and embeds the
  optional mono sample layer when present. Presets from the removed
  ADSR/ring-modulation engine are not supported.
- Use the arrow buttons, Command-Z, and Shift-Command-Z to move through up to
  30 parameter snapshots. Loading a preset file or reference starts a new
  history, while factory-preset changes remain undoable; sample replacement is
  not independently undoable.
- Click **IMPORT**, or drop a WAV anywhere on the editor. Kiq downmixes it,
  finds a likely low-frequency percussive onset, displays its waveform/pitch,
  fits the trajectories and physical controls, and extracts a short transient
  into the sample layer. **FIT** reapplies the estimated model and **ALIGN**
  applies its fundamental phase and lock-time estimate.
- Click **EXPORT / DRAG** to save a deterministic 48 kHz mono, 24-bit PCM WAV.
  Drag the same button instead to offer a freshly rendered WAV to a DAW or file
  target that accepts file-path drops.

Kicks are one-shots. MIDI note number does not transpose the trajectory;
velocity changes level, and note-off lets the current decay finish. The
standalone currently routes note-on and velocity only; pitch bend and MIDI CC
control are not connected to the audio engine yet. Reference matching is a
deterministic heuristic, not an exact recreation or stem separator, and the
sample layer contains only its extracted transient rather than full reference
playback. LOOP is driven by Kiq's audio clock, not MIDI clock or DAW transport.
The limiter is a smooth ceiling-scaled clipper rather than a look-ahead
dynamics processor.
