#!/bin/bash

# Run script for Kick Drum Synthesizer Standalone App

APP_PATH="build/bin/KickDrumSynthStandalone.app/Contents/MacOS/KickDrumSynthStandalone"

if [ ! -f "$APP_PATH" ]; then
    echo "Standalone app not found. Building..."
    ./scripts/build/build_standalone.sh
fi

echo "=== Running Kick Drum Synthesizer ==="
echo ""

"$APP_PATH"
