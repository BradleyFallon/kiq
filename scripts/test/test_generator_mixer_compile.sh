#!/bin/bash

# Simple compilation test for GeneratorMixer
# This verifies the code compiles and basic functionality works

echo "=== Testing GeneratorMixer Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/modulation/GeneratorMixer.cpp $TEMP_DIR/
cp src/audio_engine/modulation/GeneratorMixer.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "GeneratorMixer.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace KickDrum;

void testBasicFunctionality() {
    GeneratorMixer mixer;
    
    // Test default levels
    assert(mixer.getSineLevel() == 0.0f);
    assert(mixer.getHarmonicLevel() == 0.0f);
    assert(mixer.getNoiseLevel() == 0.0f);
    std::cout << "✓ Default levels are 0.0" << std::endl;
    
    // Test setters and getters
    mixer.setSineLevel(0.5f);
    assert(mixer.getSineLevel() == 0.5f);
    mixer.setHarmonicLevel(0.3f);
    assert(mixer.getHarmonicLevel() == 0.3f);
    mixer.setNoiseLevel(0.7f);
    assert(mixer.getNoiseLevel() == 0.7f);
    std::cout << "✓ Setters and getters work correctly" << std::endl;
    
    // Test clamping above range
    mixer.setSineLevel(1.5f);
    assert(mixer.getSineLevel() == 1.0f);
    mixer.setHarmonicLevel(2.0f);
    assert(mixer.getHarmonicLevel() == 1.0f);
    mixer.setNoiseLevel(10.0f);
    assert(mixer.getNoiseLevel() == 1.0f);
    std::cout << "✓ Level clamping above range works" << std::endl;
    
    // Test clamping below range
    mixer.setSineLevel(-0.5f);
    assert(mixer.getSineLevel() == 0.0f);
    mixer.setHarmonicLevel(-1.0f);
    assert(mixer.getHarmonicLevel() == 0.0f);
    mixer.setNoiseLevel(-10.0f);
    assert(mixer.getNoiseLevel() == 0.0f);
    std::cout << "✓ Level clamping below range works" << std::endl;
    
    // Test mixing with zero levels produces silence
    float output = mixer.mix(0.5f, 0.3f, 0.7f);
    assert(std::abs(output - 0.0f) < 0.0001f);
    std::cout << "✓ Mixing with zero levels produces silence" << std::endl;
    
    // Test mixing with only sine level
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(0.0f);
    mixer.setNoiseLevel(0.0f);
    output = mixer.mix(0.5f, 0.3f, 0.7f);
    assert(std::abs(output - 0.5f) < 0.0001f);
    std::cout << "✓ Mixing with only sine level works" << std::endl;
    
    // Test mixing with only harmonic level
    mixer.setSineLevel(0.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(0.0f);
    output = mixer.mix(0.5f, 0.3f, 0.7f);
    assert(std::abs(output - 0.3f) < 0.0001f);
    std::cout << "✓ Mixing with only harmonic level works" << std::endl;
    
    // Test mixing with only noise level
    mixer.setSineLevel(0.0f);
    mixer.setHarmonicLevel(0.0f);
    mixer.setNoiseLevel(1.0f);
    output = mixer.mix(0.5f, 0.3f, 0.7f);
    assert(std::abs(output - 0.7f) < 0.0001f);
    std::cout << "✓ Mixing with only noise level works" << std::endl;
    
    // Test mixing with all levels at 1.0
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(1.0f);
    output = mixer.mix(0.2f, 0.3f, 0.1f);
    float expected = 0.2f + 0.3f + 0.1f;
    assert(std::abs(output - expected) < 0.0001f);
    std::cout << "✓ Mixing with all levels at 1.0 works" << std::endl;
    
    // Test mixing with partial levels
    mixer.setSineLevel(0.5f);
    mixer.setHarmonicLevel(0.3f);
    mixer.setNoiseLevel(0.8f);
    output = mixer.mix(0.4f, 0.6f, 0.2f);
    expected = (0.4f * 0.5f) + (0.6f * 0.3f) + (0.2f * 0.8f);
    assert(std::abs(output - expected) < 0.0001f);
    std::cout << "✓ Mixing with partial levels works" << std::endl;
    
    // Test mixing with negative inputs
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(1.0f);
    output = mixer.mix(-0.5f, -0.3f, -0.2f);
    expected = -0.5f + (-0.3f) + (-0.2f);
    assert(std::abs(output - expected) < 0.0001f);
    std::cout << "✓ Mixing with negative inputs works" << std::endl;
    
    // Test mixing formula correctness
    mixer.setSineLevel(0.6f);
    mixer.setHarmonicLevel(0.4f);
    mixer.setNoiseLevel(0.2f);
    output = mixer.mix(1.0f, 0.5f, -0.5f);
    expected = (1.0f * 0.6f) + (0.5f * 0.4f) + (-0.5f * 0.2f);
    // Expected: 0.6 + 0.2 + (-0.1) = 0.7
    assert(std::abs(output - 0.7f) < 0.0001f);
    std::cout << "✓ Mixing formula correctness verified" << std::endl;
    
    // Test independent level control
    mixer.setSineLevel(0.5f);
    mixer.setHarmonicLevel(0.3f);
    mixer.setNoiseLevel(0.7f);
    mixer.setSineLevel(0.9f);
    assert(mixer.getSineLevel() == 0.9f);
    assert(mixer.getHarmonicLevel() == 0.3f);
    assert(mixer.getNoiseLevel() == 0.7f);
    std::cout << "✓ Independent level control works" << std::endl;
}

int main() {
    std::cout << "Testing GeneratorMixer implementation..." << std::endl;
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_generator_mixer \
    $TEMP_DIR/GeneratorMixer.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_generator_mixer
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== GeneratorMixer implementation verified! ==="
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
