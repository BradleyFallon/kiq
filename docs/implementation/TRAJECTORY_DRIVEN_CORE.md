# Trajectory-driven kick core

## Model

`KickParams` is the single source of synthesis defaults and parameter metadata.
It contains four absolute-Hz membrane-tension points, four dB energy-decay
points, strike position, independent impact/air controls, and output gain.
Point zero always occurs at 0 ms; later point times are sanitized into ascending
order.

`Trajectory` evaluates fixed-size point arrays. Energy decay interpolates in
dB. Membrane tension is represented by its resulting fundamental frequency; it
interpolates `log(frequency)` and exponentiates the result, so equal fractions
describe frequency ratios rather than arbitrary Hz distances. The curve value
on each starting point shapes that segment while preserving exact endpoint
values.

`MembraneModel` renders the fundamental plus two damped upper modes. Their
frequency ratios come from the first roots of the ideal circular membrane's
order-0, order-1, and order-2 modes. Strike position evaluates the matching
Bessel mode shapes from the center toward (but not onto) the fixed rim: a
center hit suppresses the non-axisymmetric upper modes, while moving outward
changes their coupling and reduces the fundamental. This is a compact modal
model, not a full membrane simulation.

## Rendering

On trigger, `Voice` resets all three modal responses to their force-impulse zero
crossing and resets the deterministic air-noise generator. For every sample it:

1. Converts sample age to milliseconds.
2. Evaluates instantaneous membrane tension and dB energy decay.
3. Advances the three damped membrane modes and applies the energy envelope.
4. Adds a smooth, finite-contact impact wavelet and a band-limited air burst.
5. Applies velocity and output gain; the engine then uses a continuous,
   monotonic soft-clip shoulder at its output.

Beater hardness controls the impact contact time and the air transient's upper
bandwidth. Air decay controls its deterministic exponential envelope. The
impact wavelet has positive and negative lobes with zero endpoints, avoiding
the sample-rate-dependent discontinuity of the former two-sample click.

After the final energy-decay point, the membrane body gets a five-millisecond
raised-cosine tail so even a deliberately loud endpoint reaches zero without a
hard edge. The air transient keeps its independent duration. MIDI notes
identify overlapping hits but do not retune the absolute tension trajectory.
Note-off is intentionally ignored; all-notes-off remains a hard stop.

Each trigger snapshots its tension, energy, membrane, and transient settings.
Later edits affect the next hit instead of stepping a currently ringing voice.
Output gain is the exception: it stays live and ramps to its new value over
five milliseconds.

## Parameters and hosts

`kKickParameterSpecs` defines 26 IDs, keys, names, ranges, and units. The
`ParameterManager`, `AudioEngine`, standalone command list, editor, and VST3
mapping all derive their values from this model. The old 29-parameter ADSR,
harmonic, ring-modulation, compressor, and reverb contract is not supported.

The editor's response preview constructs a `Voice` from the current parameter
snapshot, renders it at 48 kHz, and applies the same soft clip as the audio
engine. It draws a thin per-time-bin peak waveform and a time-aligned,
log-scaled fundamental-tuning curve. LOOP and its 40–240 BPM tempo are
editor-only controls, but repeats are scheduled by the audio engine at exact
sample offsets. They are not host parameters or synchronized to DAW transport.

## Verification

Tests cover exact node values, positive and monotonic default tension,
log-frequency interpolation, timing agreement at 48 and 96 kHz, deterministic
modal/noise reset, circular-mode center-strike behavior,
strike-position-dependent modal content, sample-rate-stable impact duration, a
smooth transient onset and endpoint, 52 Hz settling, identical repeat renders,
one-shot note-off behavior, sample-accurate event timing and audition looping,
stable per-hit parameter snapshots, smoothed live output, and continuous
monotonic soft clipping. Steinberg's VST3 validator is also run by the plugin
build.
