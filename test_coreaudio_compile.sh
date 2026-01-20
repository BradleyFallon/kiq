#!/bin/bash

# Test script for CoreAudio integration compilation
# This verifies that the CoreAudio interface compiles correctly

set -e

echo "=== Testing CoreAudio Integration Compilation ==="
echo ""

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "This test is only available on macOS"
    exit 0
fi

# Build directory
BUILD_DIR="build"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build the test
echo ""
echo "Building CoreAudio integration test..."
make -j$(sysctl -n hw.ncpu)

echo ""
echo "=== Build Successful ==="
echo ""
echo "To run the manual test:"
echo "  ./build/tests/manual/test_coreaudio_integration"
echo ""
echo "Note: This will play audio through your default output device."
echo "      Make sure your volume is at a reasonable level!"
