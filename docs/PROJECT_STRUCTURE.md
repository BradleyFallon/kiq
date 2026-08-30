# Project structure

```text
src/
  audio_engine/
    core/          AudioEngine coordination and output safety
    envelopes/     Four-point linear/log trajectory evaluation
    generators/    Sine body, deterministic noise, click/noise transient
    parameters/    Authoritative KickParams model and host parameters
    voice/         One-shot voice and four-hit overlap allocator
    midi/          MIDI parsing, triggering, CC mapping, pitch bend
    presets/       Generic JSON preset persistence
    utils/         DSP and JSON helpers
  platform/        macOS CoreAudio, CoreMIDI, and filesystem adapters
  standalone/      Terminal-driven macOS application
  vst3/            Raw Steinberg VST3 processor/controller wrapper
  ui/              Existing UI placeholders; not connected to a product UI
tests/
  unit/            Google Test behavior and component coverage
  manual/          Small executable integration checks
  property/        Reserved property-test package
external/vst3sdk/  Local Steinberg SDK checkout (ignored by Git)
```

`KickParams.h` owns parameter identity, ranges, and defaults. The standalone,
engine, preset layer, and VST3 mapping consume that model instead of defining
their own synthesis defaults.
