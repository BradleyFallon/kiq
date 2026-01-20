#!/bin/bash

# Simple compilation test for Parameter
# This verifies the code compiles and basic functionality works

echo "=== Testing Parameter Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/parameters/Parameter.cpp $TEMP_DIR/
cp src/audio_engine/parameters/Parameter.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "Parameter.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace KickDrum;

bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void testBasicFunctionality() {
    // Create a test parameter: Base Pitch (20Hz to 200Hz, default 50Hz)
    Parameter basePitch("basePitch", "Base Pitch", 50.0f, 20.0f, 200.0f, "Hz");
    
    // Test constructor and getters
    assert(basePitch.getId() == "basePitch");
    assert(basePitch.getName() == "Base Pitch");
    assert(basePitch.getValue() == 50.0f);
    assert(basePitch.getDefaultValue() == 50.0f);
    assert(basePitch.getMinValue() == 20.0f);
    assert(basePitch.getMaxValue() == 200.0f);
    assert(basePitch.getUnit() == "Hz");
    std::cout << "✓ Constructor and getters work" << std::endl;
    
    // Test setValue and getValue
    basePitch.setValue(100.0f);
    assert(basePitch.getValue() == 100.0f);
    std::cout << "✓ setValue and getValue work" << std::endl;
    
    // Test value clamping to minimum
    basePitch.setValue(10.0f);
    assert(basePitch.getValue() == 20.0f);
    std::cout << "✓ Value clamping to minimum works" << std::endl;
    
    // Test value clamping to maximum
    basePitch.setValue(250.0f);
    assert(basePitch.getValue() == 200.0f);
    std::cout << "✓ Value clamping to maximum works" << std::endl;
    
    // Test normalization
    basePitch.setValue(20.0f);
    assert(basePitch.normalize() == 0.0f);
    
    basePitch.setValue(200.0f);
    assert(basePitch.normalize() == 1.0f);
    
    basePitch.setValue(110.0f);
    assert(basePitch.normalize() == 0.5f);
    std::cout << "✓ Normalization works" << std::endl;
    
    // Test denormalization
    basePitch.denormalize(0.0f);
    assert(basePitch.getValue() == 20.0f);
    
    basePitch.denormalize(1.0f);
    assert(basePitch.getValue() == 200.0f);
    
    basePitch.denormalize(0.5f);
    assert(basePitch.getValue() == 110.0f);
    std::cout << "✓ Denormalization works" << std::endl;
    
    // Test denormalization clamping
    basePitch.denormalize(1.5f);
    assert(basePitch.getValue() == 200.0f);
    
    basePitch.denormalize(-0.5f);
    assert(basePitch.getValue() == 20.0f);
    std::cout << "✓ Denormalization clamping works" << std::endl;
    
    // Test round-trip normalization/denormalization
    basePitch.setValue(75.0f);
    float normalized = basePitch.normalize();
    Parameter temp("temp", "Temp", 50.0f, 20.0f, 200.0f, "Hz");
    temp.denormalize(normalized);
    assert(approxEqual(temp.getValue(), 75.0f));
    std::cout << "✓ Round-trip normalization/denormalization works" << std::endl;
    
    // Test reset
    basePitch.setValue(150.0f);
    basePitch.reset();
    assert(basePitch.getValue() == 50.0f);
    std::cout << "✓ Reset works" << std::endl;
    
    // Test isDefault
    assert(basePitch.isDefault());
    basePitch.setValue(100.0f);
    assert(!basePitch.isDefault());
    basePitch.reset();
    assert(basePitch.isDefault());
    std::cout << "✓ isDefault works" << std::endl;
}

void testEdgeCases() {
    // Test default constructor
    Parameter param;
    assert(param.getId() == "");
    assert(param.getValue() == 0.0f);
    std::cout << "✓ Default constructor works" << std::endl;
    
    // Test min == max
    Parameter fixed("fixed", "Fixed", 5.0f, 5.0f, 5.0f, "");
    assert(fixed.getValue() == 5.0f);
    assert(fixed.normalize() == 0.0f);
    fixed.denormalize(0.5f);
    assert(fixed.getValue() == 5.0f);
    std::cout << "✓ Min equals max edge case works" << std::endl;
    
    // Test min > max (should swap)
    Parameter swapped("swapped", "Swapped", 50.0f, 100.0f, 0.0f, "");
    assert(swapped.getMinValue() == 0.0f);
    assert(swapped.getMaxValue() == 100.0f);
    std::cout << "✓ Min > max swapping works" << std::endl;
    
    // Test default value outside range
    Parameter clamped("clamped", "Clamped", 300.0f, 20.0f, 200.0f, "Hz");
    assert(clamped.getDefaultValue() == 200.0f);
    assert(clamped.getValue() == 200.0f);
    std::cout << "✓ Default value clamping works" << std::endl;
    
    // Test negative range
    Parameter db("threshold", "Threshold", -12.0f, -60.0f, 0.0f, "dB");
    assert(db.getValue() == -12.0f);
    assert(db.normalize() == 0.8f);
    db.denormalize(0.5f);
    assert(db.getValue() == -30.0f);
    std::cout << "✓ Negative range works" << std::endl;
}

void testPercentageParameter() {
    Parameter level("level", "Level", 50.0f, 0.0f, 100.0f, "%");
    
    level.setValue(25.0f);
    assert(level.getValue() == 25.0f);
    assert(level.normalize() == 0.25f);
    
    level.denormalize(0.75f);
    assert(level.getValue() == 75.0f);
    std::cout << "✓ Percentage parameter works" << std::endl;
}

int main() {
    std::cout << "Testing Parameter implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testBasicFunctionality();
        std::cout << std::endl;
        testEdgeCases();
        std::cout << std::endl;
        testPercentageParameter();
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_parameter \
    $TEMP_DIR/Parameter.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_parameter
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== Parameter implementation verified! ==="
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
