#!/bin/bash

# Simple compilation test for DSPUtils
# This verifies the code compiles and basic functionality works

echo "=== Testing DSPUtils Implementation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/utils/DSPUtils.cpp $TEMP_DIR/
cp src/audio_engine/utils/DSPUtils.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "DSPUtils.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <limits>

using namespace KickDrum::DSPUtils;

bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void testSoftClipping() {
    std::cout << "Testing soft clipping..." << std::endl;
    
    // Test pass-through for small signals
    assert(approxEqual(softClip(0.0f), 0.0f));
    assert(approxEqual(softClip(0.5f), 0.5f));
    assert(approxEqual(softClip(-0.5f), -0.5f));
    std::cout << "  ✓ Small signals pass through unchanged" << std::endl;
    
    // Test limiting for large signals
    assert(approxEqual(softClip(1.5f), 1.0f));
    assert(approxEqual(softClip(-1.5f), -1.0f));
    assert(approxEqual(softClip(10.0f), 1.0f));
    assert(approxEqual(softClip(-10.0f), -1.0f));
    std::cout << "  ✓ Large signals limited to ±1.0" << std::endl;
    
    // Test output always in range
    std::vector<float> testValues = {-10.0f, -2.0f, -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 2.0f, 10.0f};
    for (float input : testValues) {
        float output = softClip(input);
        assert(output >= -1.0f && output <= 1.0f);
    }
    std::cout << "  ✓ Output always in range [-1.0, 1.0]" << std::endl;
    
    // Test symmetry
    for (float value : {0.5f, 0.7f, 0.9f, 1.2f, 2.0f}) {
        float positive = softClip(value);
        float negative = softClip(-value);
        assert(approxEqual(positive, -negative));
    }
    std::cout << "  ✓ Soft clipping is symmetric" << std::endl;
    
    // Test buffer processing
    std::vector<float> buffer = {0.5f, 1.5f, -1.5f, 0.8f, -0.3f};
    softClipBuffer(buffer.data(), buffer.size());
    assert(approxEqual(buffer[0], 0.5f));
    assert(approxEqual(buffer[1], 1.0f));
    assert(approxEqual(buffer[2], -1.0f));
    std::cout << "  ✓ Buffer processing works" << std::endl;
}

void testNaNDetection() {
    std::cout << "Testing NaN/infinity detection..." << std::endl;
    
    // Test isValid
    assert(isValid(0.0f));
    assert(isValid(1.0f));
    assert(isValid(-1.0f));
    assert(!isValid(std::numeric_limits<float>::quiet_NaN()));
    assert(!isValid(std::numeric_limits<float>::infinity()));
    assert(!isValid(-std::numeric_limits<float>::infinity()));
    std::cout << "  ✓ isValid detects NaN and infinity" << std::endl;
    
    // Test buffer validation
    std::vector<float> validBuffer = {0.5f, 0.8f, -0.3f, 1.0f};
    assert(isBufferValid(validBuffer.data(), validBuffer.size()));
    std::cout << "  ✓ Valid buffer passes validation" << std::endl;
    
    std::vector<float> invalidBuffer = {0.5f, std::numeric_limits<float>::quiet_NaN(), 0.3f};
    size_t invalidIndex = 0;
    assert(!isBufferValid(invalidBuffer.data(), invalidBuffer.size(), &invalidIndex));
    assert(invalidIndex == 1);
    std::cout << "  ✓ Invalid buffer detected with correct index" << std::endl;
    
    // Test sanitization
    std::vector<float> dirtyBuffer = {
        0.5f,
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        0.8f
    };
    size_t count = sanitizeBuffer(dirtyBuffer.data(), dirtyBuffer.size());
    assert(count == 2);
    assert(approxEqual(dirtyBuffer[0], 0.5f));
    assert(approxEqual(dirtyBuffer[1], 0.0f));
    assert(approxEqual(dirtyBuffer[2], 0.0f));
    assert(approxEqual(dirtyBuffer[3], 0.8f));
    std::cout << "  ✓ Sanitization replaces invalid values with zero" << std::endl;
}

void testMasterLevel() {
    std::cout << "Testing master level control..." << std::endl;
    
    // Test scaling
    std::vector<float> buffer = {0.5f, 1.0f, -0.5f, -1.0f};
    applyMasterLevel(buffer.data(), buffer.size(), 0.5f);
    assert(approxEqual(buffer[0], 0.25f));
    assert(approxEqual(buffer[1], 0.5f));
    assert(approxEqual(buffer[2], -0.25f));
    assert(approxEqual(buffer[3], -0.5f));
    std::cout << "  ✓ Master level scales buffer correctly" << std::endl;
    
    // Test zero level
    std::vector<float> buffer2 = {0.5f, 1.0f, -0.5f};
    applyMasterLevel(buffer2.data(), buffer2.size(), 0.0f);
    for (float sample : buffer2) {
        assert(approxEqual(sample, 0.0f));
    }
    std::cout << "  ✓ Zero level silences buffer" << std::endl;
    
    // Test full level
    std::vector<float> original = {0.5f, 1.0f, -0.5f};
    std::vector<float> buffer3 = original;
    applyMasterLevel(buffer3.data(), buffer3.size(), 1.0f);
    for (size_t i = 0; i < buffer3.size(); ++i) {
        assert(approxEqual(buffer3[i], original[i]));
    }
    std::cout << "  ✓ Full level preserves buffer" << std::endl;
    
    // Test clamping
    std::vector<float> buffer4 = {0.5f, 1.0f};
    applyMasterLevel(buffer4.data(), buffer4.size(), -0.5f);
    assert(approxEqual(buffer4[0], 0.0f));
    assert(approxEqual(buffer4[1], 0.0f));
    std::cout << "  ✓ Negative level clamped to zero" << std::endl;
    
    std::vector<float> buffer5 = {0.5f, 1.0f};
    applyMasterLevel(buffer5.data(), buffer5.size(), 2.0f);
    assert(approxEqual(buffer5[0], 0.5f));
    assert(approxEqual(buffer5[1], 1.0f));
    std::cout << "  ✓ Excessive level clamped to 1.0" << std::endl;
}

void testUtilityFunctions() {
    std::cout << "Testing utility functions..." << std::endl;
    
    // Test clamp
    assert(approxEqual(clamp(0.5f, 0.0f, 1.0f), 0.5f));
    assert(approxEqual(clamp(-0.5f, 0.0f, 1.0f), 0.0f));
    assert(approxEqual(clamp(1.5f, 0.0f, 1.0f), 1.0f));
    std::cout << "  ✓ Clamp function works" << std::endl;
    
    // Test denormal prevention
    float result = preventDenormal(0.0f);
    assert(result != 0.0f);
    assert(std::abs(result) < 1e-20f);
    std::cout << "  ✓ Denormal prevention adds small offset" << std::endl;
}

int main() {
    std::cout << "Testing DSPUtils implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testSoftClipping();
        std::cout << std::endl;
        
        testNaNDetection();
        std::cout << std::endl;
        
        testMasterLevel();
        std::cout << std::endl;
        
        testUtilityFunctions();
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_dsp_utils \
    $TEMP_DIR/DSPUtils.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_dsp_utils
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== DSPUtils implementation verified! ==="
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
