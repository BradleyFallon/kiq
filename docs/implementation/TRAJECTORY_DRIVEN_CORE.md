# Trajectory-driven kick core

## Model

`KickParams` is the single source of synthesis defaults and parameter metadata.
It contains four absolute-Hz membrane-tension points, four dB energy-decay
points, independent membrane/impact/air/sample levels, strike and beater
controls, fundamental phase and optional phase-lock time, output gain, and the
low/mid/high EQ, saturation, and limiter-ceiling settings. The immutable mono
sample buffer itself lives beside `KickParams`; `sampleLevel` is the automatable
parameter that brings it into the four-layer mix. Point zero always occurs at
0 ms; later point times are sanitized into ascending order.

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

Fundamental phase is expressed in degrees. With phase lock disabled it is the
phase at onset. With phase lock enabled, `Voice` integrates the curved pitch
trajectory and chooses the onset phase so the fundamental reaches the requested
phase at the chosen lock time. The two upper modes keep their own deterministic
zero-phase trigger.

## Rendering

On trigger, `Voice` resets the three modal responses to their configured
deterministic phases, resets the air-noise generator, and captures the current
physical parameters and optional sample layer. For every sample it:

1. Converts sample age to milliseconds.
2. Evaluates instantaneous membrane tension and dB energy decay.
3. Advances the three damped membrane modes and applies the energy envelope.
4. Renders the smooth finite-contact impact and band-limited air layers.
5. Linearly resamples the optional sample layer at its recorded source rate.
6. Mixes the membrane, impact, air, and sample layers, then applies velocity and
   output gain.

After voices mix, `AudioEngine` splits the signal around 120 Hz and 3 kHz for
low/mid/high gain, blends in tanh saturation, and applies a ceiling-scaled,
continuous soft-clip shoulder. Output-stage targets are smoothed over a short
ramp. This limiter is deliberately a deterministic soft ceiling, not a
look-ahead compressor or brick-wall dynamics stage.

Beater hardness controls the impact contact time and the air transient's upper
bandwidth. Air decay controls its deterministic exponential envelope. The
impact wavelet has positive and negative lobes with zero endpoints, avoiding
the sample-rate-dependent discontinuity of the former two-sample click.
The optional sample layer gets a two-millisecond end fade so resampling cannot
expose a non-zero final source sample as a discontinuity.

After the final energy-decay point, the membrane body gets a five-millisecond
raised-cosine tail so even a deliberately loud endpoint reaches zero without a
hard edge. The air transient keeps its independent duration. MIDI notes
identify overlapping hits but do not retune the absolute tension trajectory.
Note-off is intentionally ignored; all-notes-off remains a hard stop.

Each trigger snapshots its tension, energy, membrane, transient, phase, layer
levels, and sample choice. Later edits to those controls affect the next hit
instead of stepping a currently ringing voice. Output gain is the per-voice
exception: it stays live and ramps to its new value over five milliseconds.
The global EQ, saturation, and limiter targets also remain live and smoothed.

## Parameters and hosts

`kKickParameterSpecs` defines 35 IDs, keys, names, ranges, and units. The
`ParameterManager`, `AudioEngine`, shared editor, and VST3 mapping all derive
their values from this model. The editor exposes the layer and physical
controls on its **MODEL** page and the EQ, saturation, limiter, and output gain
on its **OUTPUT** page. The old 29-parameter ADSR, harmonic, ring-modulation,
compressor, and reverb contract is not supported.

The VST3 processor keeps host automation points and note events in bounded,
fixed-storage block queues, sorts them by sample offset, and renders between
event boundaries. Parameter changes precede notes at the same offset, so a new
hit snapshots the exact host state for that sample. Points at the block end
establish next-block state using the same ordering. If a pathological block
exceeds the fixed automation capacity, Kiq still preserves the latest value of
each overflowed parameter as next-block state; excess intermediate points (and
note events beyond the separate note capacity) cannot be retained. The optional
sample layer is sent from controller to processor and is included in VST3
component state.

The editor's response preview performs a deterministic 48 kHz offline render
through the complete `AudioEngine`, including the optional sample and output
stage. It draws a thin per-time-bin waveform and a time-aligned, log-scaled
fundamental-tuning curve. WAV export uses the same path and writes mono 24-bit
PCM; the **EXPORT / DRAG** control can save it or expose the rendered file to a
host that accepts file-path drops.

Current `.kiqpreset` files use the strict `kiq-kick-1` schema, contain all 35
parameters, and can embed the mono float sample plus source-path metadata.
Legacy or partial parameter maps are rejected instead of being interpreted as
the current engine. The editor also provides seven factory presets and a
bounded 30-step history of complete sanitized `KickParams` snapshots. History
does not own sample buffers, so loading a preset file or importing a reference
starts a new history. Factory-preset changes remain normal undoable snapshots.

LOOP and its 40–240 BPM tempo are editor-only controls, but repeats are
scheduled by the audio engine at exact sample offsets. They are not host
parameters or synchronized to DAW transport.

## Reference matching

The reference path accepts RIFF/WAVE PCM16, PCM24, PCM32, and float32 input,
downmixes channels to bounded mono, and can search a longer file for a likely
low-frequency percussive onset. Analysis produces a peak-preserving waveform,
pitch and amplitude estimates, a transient/body separation, a fitted
`KickParams`-shaped physical model, and an optional short transient sample.

The shared editor overlays the reference waveform and confident pitch points,
applies the physical fit on import, and retains it for **FIT** and **ALIGN**.
**ALIGN** sets the fundamental phase and lock time from the analysis. This is a
deterministic signal-analysis heuristic intended to create a useful starting
point; it is not source separation, machine-learned matching, or an exact
inverse of the synthesizer.

## Verification

Tests cover exact node values, positive and monotonic default tension,
log-frequency interpolation, timing agreement at 48 and 96 kHz, deterministic
modal/noise reset, circular-mode center-strike behavior,
strike-position-dependent modal content, sample-rate-stable impact duration, a
smooth transient onset and endpoint, independent layer levels, optional sample
resampling, phase rotation and phase-lock accuracy, 52 Hz settling, identical
repeat renders, one-shot note-off behavior, sample-accurate parameter/note
ordering and audition looping, stable per-hit snapshots, smoothed output EQ and
saturation, limiter behavior, WAV decoding and physical fitting, strict preset
and embedded-sample round trips, bounded undo/redo, and deterministic mono
24-bit export. Steinberg's VST3 validator is also run by the plugin build.
