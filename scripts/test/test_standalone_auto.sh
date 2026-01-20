#!/bin/bash

# Automated test script for standalone app
# This script sends commands automatically to test the synth

APP_PATH="build/bin/KickDrumSynthStandalone.app/Contents/MacOS/KickDrumSynthStandalone"

echo "=== Automated Standalone App Test ==="
echo ""
echo "This will play several test notes automatically."
echo "You should hear kick drum sounds!"
echo ""

# Send commands to the app
{
    sleep 2
    echo "play 60 0.8"
    sleep 1.5
    echo "play 48 1.0"
    sleep 1.5
    echo "play 72 0.6"
    sleep 1.5
    echo "param basePitch 40.0"
    echo "play 60 0.9"
    sleep 2
    echo "quit"
} | "$APP_PATH"

echo ""
echo "Test complete!"
