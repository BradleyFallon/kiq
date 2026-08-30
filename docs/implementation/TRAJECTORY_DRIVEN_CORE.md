# Trajectory-driven kick core

## Model

`KickParams` is the single source of synthesis defaults and parameter metadata.
It contains four absolute-Hz pitch points, four dB amplitude points, a phase
reset, independent transient controls, and output gain. Point zero always
occurs at 0 ms; later point times are sanitized into ascending order.

`Trajectory` evaluates fixed-size point arrays. Amplitude interpolates in dB.
Pitch interpolates `log(frequency)` and exponentiates the result, so equal
fractions describe frequency ratios rather than arbitrary Hz distances. The
curve value on each starting point shapes that segment while preserving exact
endpoint values.

## Rendering

On trigger, `Voice` resets the sine phase and deterministic noise generator.
For every sample it:

1. Converts sample age to milliseconds.
2. Evaluates instantaneous pitch and dB amplitude.
3. Advances one sine oscillator at that frequency.
4. Adds an independent two-sample click and low-pass-filtered noise burst.
5. Applies velocity and output gain.

The voice ends at the final amplitude point. MIDI notes identify overlapping
hits but do not retune the absolute pitch trajectory. Note-off is intentionally
ignored; all-notes-off remains a hard stop.

## Parameters and hosts

`kKickParameterSpecs` defines 26 IDs, keys, names, ranges, and units. The
`ParameterManager`, `AudioEngine`, standalone command list, and VST3 mapping all
derive their values from this model. The old 29-parameter ADSR, harmonic,
ring-modulation, compressor, and reverb contract is not supported.

## Verification

Tests require exact node values, positive and monotonic default pitch,
log-frequency interpolation, timing agreement at 48 and 96 kHz, deterministic
phase/noise reset, 52 Hz settling, transient completion before body completion,
identical repeat renders, one-shot note-off behavior, and sample-accurate host
parameter application. Steinberg's VST3 validator is also run by the plugin
build.
