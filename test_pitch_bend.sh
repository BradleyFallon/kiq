#!/bin/bash

# Test script for MIDI pitch bend functionality

set -e

echo "========================================="
echo "Testing MIDI Pitch Bend Implementation"
echo "========================================="
echo ""

# Build the tests
echo "Building tests..."
cmake --build build --target kick_drum_tests

echo ""
echo "Running pitch bend tests..."
echo ""

# Run the pitch bend tests
./build/bin/kick_drum_tests --gtest_filter="*PitchBend*"

echo ""
echo "========================================="
echo "All pitch bend tests passed!"
echo "========================================="

