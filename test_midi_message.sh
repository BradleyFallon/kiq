#!/bin/bash

# Test script for MIDI message parsing
# Tests task 13.1: Implement MIDI message parsing

set -e

echo "=========================================="
echo "Testing MIDI Message Parsing (Task 13.1)"
echo "=========================================="
echo ""

# Build the tests
echo "Building tests..."
cmake --build build --target kick_drum_tests

echo ""
echo "Running MIDI message tests..."
echo ""

# Run the MIDI message tests
./build/bin/kick_drum_tests --gtest_filter="MIDIMessageTest.*"

echo ""
echo "=========================================="
echo "All MIDI message tests passed!"
echo "=========================================="
