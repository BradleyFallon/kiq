#!/bin/bash

# Simple compilation test for HarmonicMembrane
# This verifies the code compiles and basic functionality works

echo "=== Testing HarmonicMembrane Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/generators/HarmonicMembrane.cpp $TEMP_DIR/
cp src/audio_engine/generators/HarmonicMembrane.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "HarmonicMembrane.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace KickDrum;

bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void testBasicFunctionality() {
    HarmonicMembrane membrane;
    const float sampleRate = 48000.0f;
    const float baseFrequency = 100.0f;
    
    // Test initialization
    assert(!membrane.isInitialized());
    membrane.initialize(sampleRate);
    assert(membrane.isInitialized());
    std::cout << "✓ Initialization works" << std::endl;
    
    // Test base frequency setting
    membrane.setBaseFrequency(baseFrequency);
    assert(membrane.getBaseFrequency() == baseFrequency);
    std::cout << "✓ Base frequency setting works" << std::endl;
    
    // Test negative base frequency clamping
    membrane.setBaseFrequency(-100.0f);
    assert(membrane.getBaseFrequency() == 0.0f);
    std::cout << "✓ Negative base frequency clamping works" << std::endl;
    
    // Test ratio setting
    membrane.setBaseFrequency(baseFrequency);
    membrane.setRatio(1.0f);
    assert(membrane.getRatio() == 1.0f);
    membrane.setRatio(2.0f);
    assert(membrane.getRatio() == 2.0f);
    membrane.setRatio(0.5f);
    assert(membrane.getRatio() == 0.5f);
    std::cout << "✓ Ratio setting works" << std::endl;
    
    // Test ratio clamping to minimum (0.5)
    membrane.setRatio(0.1f);
    assert(membrane.getRatio() == 0.5f);
    membrane.setRatio(0.0f);
    assert(membrane.getRatio() == 0.5f);
    std::cout << "✓ Ratio clamping to minimum (0.5) works" << std::endl;
    
    // Test ratio clamping to maximum (8.0)
    membrane.setRatio(10.0f);
    assert(membrane.getRatio() == 8.0f);
    membrane.setRatio(100.0f);
    assert(membrane.getRatio() == 8.0f);
    std::cout << "✓ Ratio clamping to maximum (8.0) works" << std::endl;
    
    // Test frequency calculation
    membrane.setBaseFrequency(100.0f);
    membrane.setRatio(1.0f);
    assert(membrane.getFrequency() == 100.0f);
    membrane.setRatio(2.0f);
    assert(membrane.getFrequency() == 200.0f);
    membrane.setRatio(0.5f);
    assert(membrane.getFrequency() == 50.0f);
    std::cout << "✓ Frequency calculation (base × ratio) works" << std::endl;
    
    // Test generate produces valid range
    membrane.setBaseFrequency(100.0f);
    membrane.setRatio(1.0f);
    for (int i = 0; i < 1000; ++i) {
        float sample = membrane.generate();
        assert(sample >= -1.0f && sample <= 1.0f);
        assert(!std::isnan(sample) && !std::isinf(sample));
    }
    std::cout << "✓ Generate produces valid range [-1.0, 1.0]" << std::endl;
    
    // Test reset
    membrane.reset();
    float sample = membrane.generate();
    assert(approxEqual(sample, 0.0f, 0.01f));
    std::cout << "✓ Reset works (phase returns to 0)" << std::endl;
    
    // Test tracking base frequency changes
    membrane.setRatio(2.0f);
    membrane.setBaseFrequency(50.0f);
    assert(membrane.getFrequency() == 100.0f);
    membrane.setBaseFrequency(100.0f);
    assert(membrane.getFrequency() == 200.0f);
    membrane.setBaseFrequency(75.0f);
    assert(membrane.getFrequency() == 150.0f);
    std::cout << "✓ Tracks base frequency changes correctly" << std::endl;
    
    // Test minimum ratio frequency
    membrane.setBaseFrequency(100.0f);
    membrane.setRatio(0.5f);
    assert(membrane.getFrequency() == 50.0f);
    for (int i = 0; i < 100; ++i) {
        float sample = membrane.generate();
        assert(sample >= -1.0f && sample <= 1.0f);
    }
    std::cout << "✓ Minimum ratio (0.5x) works" << std::endl;
    
    // Test maximum ratio frequency
    membrane.setBaseFrequency(100.0f);
    membrane.setRatio(8.0f);
    assert(membrane.getFrequency() == 800.0f);
    for (int i = 0; i < 100; ++i) {
        float sample = membrane.generate();
        assert(sample >= -1.0f && sample <= 1.0f);
    }
    std::cout << "✓ Maximum ratio (8.0x) works" << std::endl;
    
    // Test zero base frequency
    membrane.setBaseFrequency(0.0f);
    membrane.setRatio(2.0f);
    assert(membrane.getFrequency() == 0.0f);
    membrane.reset(); // Reset phase to 0
    for (int i = 0; i < 100; ++i) {
        float sample = membrane.generate();
        // With zero frequency, phase increment is 0, so phase stays at 0
        // sin(0) = 0, so all samples should be 0
        assert(approxEqual(sample, 0.0f, 0.001f));
    }
    std::cout << "✓ Zero base frequency produces silence" << std::endl;
}

int main() {
    std::cout << "Testing HarmonicMembrane implementation..." << std::endl;
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_harmonic_membrane \
    $TEMP_DIR/HarmonicMembrane.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_harmonic_membrane
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== HarmonicMembrane implementation verified! ==="
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
