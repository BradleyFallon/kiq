# Standalone app

Build the macOS app:

```bash
cmake -S . -B build -DBUILD_VST3=OFF -DBUILD_STANDALONE=ON
cmake --build build --target KickDrumSynthStandalone -j
open build/bin/KickDrumSynthStandalone.app
```

The app opens the default CoreAudio output and first available MIDI input. It
then accepts terminal commands:

```text
play <note> [velocity]
stop <note>
param <name> <value>
list
midi
help
quit
```

Examples:

```text
play 36 1.0
param pitch0Hz 260
param pitch2Hz 48
param noiseDecayMs 10
param clickLevel 0.25
```

`list` prints the authoritative parameter names and ranges. A kick is a
one-shot, so `stop`/MIDI note-off does not cut its decay short.
