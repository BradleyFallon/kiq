#!/bin/bash

# Build script for Kick Drum Synthesizer Standalone App

set -e  # Exit on error

echo "=== Building Kick Drum Synthesizer Standalone App ==="
echo ""

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_VST3=OFF \
    -DBUILD_STANDALONE=ON \
    -DBUILD_TESTS=OFF

# Build the standalone app
echo ""
echo "Building standalone application..."
cmake --build . --target KickDrumSynthStandalone -j$(sysctl -n hw.ncpu)

echo ""
echo "=== Build Complete ==="
echo ""
echo "To run the standalone app:"
echo "  ./build/src/standalone/KickDrumSynthStandalone"
echo ""
echo "Or use the run script:"
echo "  ./run_standalone.sh"
echo ""
