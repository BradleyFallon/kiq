#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
VST_BUILD_DIR="$PROJECT_DIR/cmake-build-vst3-xcode"

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_VST3=OFF \
    -DBUILD_STANDALONE=ON \
    -DBUILD_TESTS=ON
cmake --build "$BUILD_DIR" --config Release -j "$(sysctl -n hw.ncpu)"
ctest --test-dir "$BUILD_DIR" --output-on-failure

if [ -d "$PROJECT_DIR/external/vst3sdk" ]; then
    cmake -G Xcode -S "$PROJECT_DIR" -B "$VST_BUILD_DIR" \
        -DBUILD_VST3=ON \
        -DBUILD_STANDALONE=OFF \
        -DBUILD_TESTS=OFF
    cmake --build "$VST_BUILD_DIR" --config Release --target KickDrumSynth \
        -j "$(sysctl -n hw.ncpu)"
else
    echo "VST3 SDK not found; skipping plugin build."
fi

echo "Kiq build and verification complete."
