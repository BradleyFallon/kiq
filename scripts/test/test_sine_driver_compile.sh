#!/bin/bash

# Simple compilation test for SineDriver
# This verifies the code compiles and basic functionality works

echo "=== Testing SineDriver Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/generators/SineDriver.cpp $TEMP_DIR/
cp src/audio_engine/generators/SineDriver.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "SineDriver.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>

using namespace KickDrum;

bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void testBasicFunctionality() {
    SineDriver driver;
    const float sampleRate = 48000.0f;
    const float testFrequency = 440.0f;
    
    // Test initialization
    assert(!driver.isInitialized());
    driver.initialize(sampleRate);
    assert(driver.isInitialized());
    std::cout << "✓ Initialization works" << std::endl;
    
    // Test frequency setting
    driver.setFrequency(testFrequency);
    assert(driver.getFrequency() == testFrequency);
    std::cout << "✓ Frequency setting works" << std::endl;
    
    // Test negative frequency clamping
    driver.setFrequency(-100.0f);
    assert(driver.getFrequency() == 0.0f);
    std::cout << "✓ Negative frequency clamping works" << std::endl;
    
    // Test generate produces valid range
    driver.setFrequency(testFrequency);
    for (int i = 0; i < 1000; ++i) {
        float sample = driver.generate();
        assert(sample >= -1.0f && sample <= 1.0f);
        assert(!std::isnan(sample) && !std::isinf(sample));
    }
    std::cout << "✓ Generate produces valid range [-1.0, 1.0]" << std::endl;
    
    // Test reset
    driver.reset();
    float sample = driver.generate();
    assert(approxEqual(sample, 0.0f, 0.01f));
    std::cout << "✓ Reset works (phase returns to 0)" << std::endl;
    
    // Test frequency accuracy
    driver.setFrequency(100.0f);
    driver.reset();
    int numSamples = static_cast<int>(sampleRate);
    std::vector<float> samples(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        samples[i] = driver.generate();
    }
    
    // Count zero crossings
    int zeroCrossings = 0;
    for (int i = 1; i < numSamples; ++i) {
        if (samples[i-1] >= 0.0f && samples[i] < 0.0f) {
            zeroCrossings++;
        }
    }
    
    // Should be approximately 100 crossings for 100 Hz
    assert(std::abs(zeroCrossings - 100) <= 2);
    std::cout << "✓ Frequency accuracy verified (100 Hz)" << std::endl;
    
    // Test phase continuity during frequency changes
    driver.setFrequency(440.0f);
    driver.reset();
    std::vector<float> continuityTest;
    for (int i = 0; i < 100; ++i) {
        continuityTest.push_back(driver.generate());
    }
    driver.setFrequency(880.0f);
    for (int i = 0; i < 100; ++i) {
        continuityTest.push_back(driver.generate());
    }
    
    // Check for discontinuities
    bool hasDiscontinuity = false;
    for (size_t i = 1; i < continuityTest.size(); ++i) {
        float diff = std::abs(continuityTest[i] - continuityTest[i-1]);
        if (diff > 0.2f) {
            hasDiscontinuity = true;
            break;
        }
    }
    assert(!hasDiscontinuity);
    std::cout << "✓ Phase continuity maintained during frequency changes" << std::endl;
}

int main() {
    std::cout << "Testing SineDriver implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testBasicFunctionality();
        std::cout << std::endl;
        std::cout << "=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
EOF

# Compile
echo ""
echo "Compiling..."
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_sine_driver \
    $TEMP_DIR/SineDriver.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_sine_driver
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== SineDriver implementation verified! ==="
        exit 0
    else
        echo "=== Tests failed ==="
        exit 1
    fi
else
    echo "✗ Compilation failed"
    rm -rf $TEMP_DIR
    exit 1
fi
