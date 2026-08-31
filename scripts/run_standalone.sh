#!/bin/bash

# Run script for the Kiq standalone app

APP_PATH="build/bin/KickDrumSynthStandalone.app/Contents/MacOS/KickDrumSynthStandalone"

if [ ! -f "$APP_PATH" ]; then
    echo "Standalone app not found. Building..."
    ./scripts/build/build_standalone.sh
fi

echo "=== Running Kiq ==="
echo ""

"$APP_PATH"
