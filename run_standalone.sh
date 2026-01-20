#!/bin/bash

# Run script for Kick Drum Synthesizer Standalone App

if [ ! -f "build/src/standalone/KickDrumSynthStandalone" ]; then
    echo "Standalone app not found. Building..."
    ./build_standalone.sh
fi

echo "=== Running Kick Drum Synthesizer ==="
echo ""

./build/src/standalone/KickDrumSynthStandalone
