#!/bin/bash

# Quick test script for standalone app
# This script sends a few commands to test basic functionality

echo "=== Quick Standalone App Test ==="
echo ""
echo "This will:"
echo "1. Start the standalone app"
echo "2. Play a test note"
echo "3. Wait 2 seconds"
echo "4. Exit"
echo ""
echo "You should hear a kick drum sound!"
echo ""
echo "Press Enter to continue..."
read

# Send commands to the app
{
    sleep 1
    echo "play 60 0.8"
    sleep 2
    echo "quit"
} | ./build/bin/KickDrumSynthStandalone.app/Contents/MacOS/KickDrumSynthStandalone

echo ""
echo "Test complete!"
