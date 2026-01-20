#!/bin/bash

# Test script for CoreMIDI integration compilation
# This script builds the CoreMIDI interface and runs unit tests

set -e  # Exit on error

echo "=== CoreMIDI Integration Compilation Test ==="
echo ""

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build the platform library (includes CoreMIDI)
echo ""
echo "Building platform library..."
cmake --build . --target kick_drum_platform

# Build unit tests
echo ""
echo "Building unit tests..."
cmake --build . --target kick_drum_tests

# Run unit tests
echo ""
echo "Running unit tests..."
./tests/unit/kick_drum_tests --gtest_filter="CoreMIDIInterface*"

echo ""
echo "=== CoreMIDI unit tests passed ==="
echo ""

# Build manual test
echo "Building manual test..."
cmake --build . --target test_coremidi_integration

echo ""
echo "=== Build successful ==="
echo ""
echo "To run the manual test (requires MIDI controller):"
echo "  ./build/tests/manual/test_coremidi_integration"
echo ""
