#!/bin/bash

# Compile and run DSPUtils tests

set -e

echo "Compiling DSPUtils tests..."

# Create build directory
mkdir -p build_test

# Get googletest path
GTEST_PREFIX=$(brew --prefix googletest)

# Compile DSPUtils
g++ -std=c++17 -c src/audio_engine/utils/DSPUtils.cpp -o build_test/DSPUtils.o -I.

# Compile test
g++ -std=c++17 -c tests/unit/utils/DSPUtilsTest.cpp -o build_test/DSPUtilsTest.o -I. -I${GTEST_PREFIX}/include

# Link
g++ -std=c++17 build_test/DSPUtils.o build_test/DSPUtilsTest.o -o build_test/test_dsp_utils -L${GTEST_PREFIX}/lib -lgtest -lgtest_main -pthread

echo "Running DSPUtils tests..."
./build_test/test_dsp_utils

echo "DSPUtils tests completed successfully!"
