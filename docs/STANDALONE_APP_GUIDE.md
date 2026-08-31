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
- Use **STRIKE POS** to move from a center strike toward the rim, changing the
  circular membrane modes. **IMPACT**, **AIR**, **AIR DECAY**, and **BEATER**
  shape the finite-contact and band-limited transient.
- The response panel re-renders the current voice after parameter changes. Its
  thin cyan line is the post-soft-clip peak waveform; the orange line is the
  time-aligned fundamental tuning curve.
- Click **LOOP** for repeated audition, then set 40–240 BPM with **TEMPO**. The
  audio engine schedules repeats at exact sample offsets, but the loop is not
  synchronized to DAW transport.
- The right-side LEDs show output peak and clip activity.

Kicks are one-shots. MIDI note number does not transpose the trajectory;
velocity changes level, and note-off lets the current decay finish. The
standalone currently routes note-on and velocity only; pitch bend and MIDI CC
control are not connected to the audio engine yet.
