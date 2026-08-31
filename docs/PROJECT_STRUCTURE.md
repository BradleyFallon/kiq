# Project structure

```text
src/
  audio_engine/
    analysis/      WAV decoding, kick selection, overlays, separation, and fit
    core/          AudioEngine event scheduling, layer mix, and output stage
    envelopes/     Four-point linear/log trajectory evaluation
    export/        Deterministic offline render and mono 24-bit WAV writing
    generators/    Three-mode membrane, impact, and deterministic air models
    include/       Public engine and immutable sample-layer data contracts
    midi/          MIDI parsing and engine-facing message handling
    parameters/    Authoritative KickParams model and host parameters
    presets/       Strict current-format parameter/sample preset persistence
    utils/         DSP and JSON helpers
    voice/         Four-layer one-shot voice and nine-hit overlap allocator
    workflow/      Bounded whole-KickParams undo/redo history
  platform/        macOS CoreAudio, CoreMIDI, and filesystem adapters
  standalone/      Graphical macOS app and shared-editor/audio bridge
  ui/              Shared VSTGUI editor and physical-model factory presets
  vst3/            Raw VST3 wrapper, state, and sample-accurate event bridge
tests/
  unit/            Google Test engine, analysis, preset, workflow, and export coverage
  manual/          Small executable integration checks
  property/        Reserved property-test package
external/vst3sdk/  Local Steinberg SDK checkout (ignored by Git)
```

`KickParams.h` owns all 35 parameter identities, ranges, and defaults. The
engine, strict preset document, shared editor, standalone bridge, and VST3
mapping consume that model instead of defining separate synthesis contracts.
The optional immutable mono sample is deliberately separate from `KickParams`:
`SampleLayerData` travels with editor/host state and presets, while
`sampleLevel` remains an ordinary automatable parameter.

The shared editor is the product UI for both graphical targets. It owns the
model/output pages, preview and reference overlays, factory/file preset
workflow, parameter history, reference WAV import/fit, and WAV export/drag.
Audio rendering, event ordering, sample-layer lifetime, and output processing
remain in `audio_engine/` so the standalone, VST3 processor, preview, and
offline exporter use one synthesis implementation.
