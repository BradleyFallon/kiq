#!/bin/bash

# Simple compilation test for RingModulator
# This verifies the code compiles without CMake

echo "=== Testing RingModulator Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/modulation/RingModulator.cpp $TEMP_DIR/
cp src/audio_engine/modulation/RingModulator.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "RingModulator.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace KickDrum;

void testBasicFunctionality() {
    RingModulator mod;
    
    // Test default depth
    assert(mod.getDepth() == 0.0f);
    std::cout << "✓ Default depth is 0.0" << std::endl;
    
    // Test setDepth
    mod.setDepth(0.5f);
    assert(mod.getDepth() == 0.5f);
    std::cout << "✓ setDepth works correctly" << std::endl;
    
    // Test clamping
    mod.setDepth(-0.5f);
    assert(mod.getDepth() == 0.0f);
    mod.setDepth(1.5f);
    assert(mod.getDepth() == 1.0f);
    std::cout << "✓ Depth clamping works" << std::endl;
    
    // Test 0% depth (fully dry)
    mod.setDepth(0.0f);
    float result = mod.process(1.0f, 0.5f);
    assert(std::abs(result - 1.0f) < 0.0001f);
    std::cout << "✓ 0% depth outputs carrier" << std::endl;
    
    // Test 100% depth (fully wet)
    mod.setDepth(1.0f);
    result = mod.process(1.0f, 0.5f);
    assert(std::abs(result - 0.5f) < 0.0001f);
    std::cout << "✓ 100% depth outputs ring modulation" << std::endl;
    
    // Test 50% depth (blend)
    mod.setDepth(0.5f);
    result = mod.process(1.0f, 0.5f);
    // Expected: 1.0 * 0.5 + (1.0 * 0.5) * 0.5 = 0.5 + 0.25 = 0.75
    assert(std::abs(result - 0.75f) < 0.0001f);
    std::cout << "✓ 50% depth blends correctly" << std::endl;
    
    // Test with sine waves
    mod.setDepth(1.0f);
    float carrier = std::sin(M_PI / 4.0f);
    float modulator = std::sin(M_PI / 4.0f);
    result = mod.process(carrier, modulator);
    float expected = carrier * modulator;
    assert(std::abs(result - expected) < 0.0001f);
    std::cout << "✓ Ring modulation with sine waves works" << std::endl;
    
    // Test with negative values
    result = mod.process(-0.5f, -0.5f);
    assert(std::abs(result - 0.25f) < 0.0001f);
    std::cout << "✓ Negative values handled correctly" << std::endl;
}

int main() {
    std::cout << "Testing RingModulator implementation..." << std::endl;
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_ring_modulator \
    $TEMP_DIR/RingModulator.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_ring_modulator
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== RingModulator implementation verified! ==="
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
