#!/bin/bash

# Test script for Preset class compilation and unit tests

set -e

echo "=== Building Preset Tests ==="

# Create build directory if it doesn't exist
mkdir -p build_test

# Compile Preset.cpp
echo "Compiling Preset.cpp..."
g++ -std=c++17 -c \
    src/audio_engine/presets/Preset.cpp \
    -I src/audio_engine \
    -o build_test/Preset.o

# Compile PresetTest.cpp with Google Test
echo "Compiling PresetTest.cpp..."
g++ -std=c++17 -c \
    tests/unit/presets/PresetTest.cpp \
    -I src/audio_engine \
    -I external/googletest/googletest/include \
    -o build_test/PresetTest.o

# Link test executable
echo "Linking test executable..."
g++ -std=c++17 \
    build_test/Preset.o \
    build_test/PresetTest.o \
    -L build/lib \
    -lgtest \
    -lgtest_main \
    -pthread \
    -o build_test/preset_test

# Run tests
echo ""
echo "=== Running Preset Tests ==="
./build_test/preset_test

echo ""
echo "=== All Preset Tests Passed! ==="
