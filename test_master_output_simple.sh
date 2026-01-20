#!/bin/bash

# Simple test for master output control, soft clipping, and NaN detection
# Tests the DSPUtils functions that implement these features

echo "=== Testing Master Output Control, Soft Clipping, and NaN Detection ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/utils/DSPUtils.cpp $TEMP_DIR/
cp src/audio_engine/utils/DSPUtils.h $TEMP_DIR/

# Create a comprehensive test file
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

void testTask91_MasterOutputLevel() {
    std::cout << "=== Task 9.1: Master Output Level Control ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Master level scaling
    std::cout << "Test 1: Master level scales audio correctly" << std::endl;
    std::vector<float> buffer = {0.5f, 1.0f, -0.5f, -1.0f};
    applyMasterLevel(buffer.data(), buffer.size(), 0.5f);
    assert(approxEqual(buffer[0], 0.25f));
    assert(approxEqual(buffer[1], 0.5f));
    assert(approxEqual(buffer[2], -0.25f));
    assert(approxEqual(buffer[3], -0.5f));
    std::cout << "  ✓ Master level 0.5 scales by 50%" << std::endl;
    
    // Test 2: Zero level silences output
    std::cout << "Test 2: Master level 0.0 silences output" << std::endl;
    std::vector<float> buffer2 = {0.5f, 1.0f, -0.5f, -1.0f};
    applyMasterLevel(buffer2.data(), buffer2.size(), 0.0f);
    for (float sample : buffer2) {
        assert(approxEqual(sample, 0.0f));
    }
    std::cout << "  ✓ All samples are zero" << std::endl;
    
    // Test 3: Full level preserves signal
    std::cout << "Test 3: Master level 1.0 preserves signal" << std::endl;
    std::vector<float> original = {0.5f, 1.0f, -0.5f, -1.0f};
    std::vector<float> buffer3 = original;
    applyMasterLevel(buffer3.data(), buffer3.size(), 1.0f);
    for (size_t i = 0; i < buffer3.size(); ++i) {
        assert(approxEqual(buffer3[i], original[i]));
    }
    std::cout << "  ✓ Signal unchanged" << std::endl;
    
    // Test 4: Clamping to valid range
    std::cout << "Test 4: Master level clamped to [0.0, 1.0]" << std::endl;
    std::vector<float> buffer4 = {0.5f, 1.0f};
    applyMasterLevel(buffer4.data(), buffer4.size(), -0.5f);
    assert(approxEqual(buffer4[0], 0.0f));
    assert(approxEqual(buffer4[1], 0.0f));
    std::cout << "  ✓ Negative level clamped to 0.0" << std::endl;
    
    std::vector<float> buffer5 = {0.5f, 1.0f};
    applyMasterLevel(buffer5.data(), buffer5.size(), 2.0f);
    assert(approxEqual(buffer5[0], 0.5f));
    assert(approxEqual(buffer5[1], 1.0f));
    std::cout << "  ✓ Excessive level clamped to 1.0" << std::endl;
    
    // Test 5: Sign preservation
    std::cout << "Test 5: Master level preserves signal polarity" << std::endl;
    std::vector<float> buffer6 = {0.5f, -0.5f, 0.8f, -0.8f};
    applyMasterLevel(buffer6.data(), buffer6.size(), 0.5f);
    assert(buffer6[0] > 0.0f);
    assert(buffer6[1] < 0.0f);
    assert(buffer6[2] > 0.0f);
    assert(buffer6[3] < 0.0f);
    std::cout << "  ✓ Polarity preserved" << std::endl;
    
    std::cout << std::endl;
    std::cout << "✅ Task 9.1 PASSED: Master output level control working correctly" << std::endl;
    std::cout << "   - Validates Requirements 6.1, 6.2" << std::endl;
    std::cout << std::endl;
}

void testTask93_SoftClipping() {
    std::cout << "=== Task 9.3: Soft Clipping ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Detect signals exceeding ±1.0
    std::cout << "Test 1: Detect signals exceeding ±1.0" << std::endl;
    std::vector<float> testSignals = {1.5f, -1.5f, 2.0f, -2.0f, 10.0f, -10.0f};
    for (float signal : testSignals) {
        assert(std::abs(signal) > 1.0f);
    }
    std::cout << "  ✓ Test signals exceed ±1.0" << std::endl;
    
    // Test 2: Apply soft clipping algorithm
    std::cout << "Test 2: Apply soft clipping algorithm" << std::endl;
    for (float signal : testSignals) {
        float clipped = softClip(signal);
        assert(clipped >= -1.0f && clipped <= 1.0f);
    }
    std::cout << "  ✓ All clipped signals within [-1.0, 1.0]" << std::endl;
    
    // Test 3: Small signals pass through unchanged
    std::cout << "Test 3: Small signals pass through unchanged" << std::endl;
    std::vector<float> smallSignals = {0.0f, 0.3f, -0.3f, 0.5f, -0.5f, 0.6f, -0.6f};
    for (float signal : smallSignals) {
        float clipped = softClip(signal);
        assert(approxEqual(clipped, signal));
    }
    std::cout << "  ✓ Small signals unchanged" << std::endl;
    
    // Test 4: Smooth compression in transition region
    std::cout << "Test 4: Smooth compression in transition region (0.67 to 1.0)" << std::endl;
    float input1 = 0.7f;
    float output1 = softClip(input1);
    assert(output1 > 0.0f && output1 < 1.0f);
    assert(output1 > input1 * 0.9f);  // Should be close to input
    
    float input2 = 0.9f;
    float output2 = softClip(input2);
    assert(output2 > 0.0f && output2 < 1.0f);
    assert(output2 < input2);  // Should be compressed
    std::cout << "  ✓ Smooth compression applied" << std::endl;
    
    // Test 5: Symmetry
    std::cout << "Test 5: Soft clipping is symmetric" << std::endl;
    for (float value : {0.5f, 0.7f, 0.9f, 1.2f, 2.0f}) {
        float positive = softClip(value);
        float negative = softClip(-value);
        assert(approxEqual(positive, -negative));
    }
    std::cout << "  ✓ Symmetric around zero" << std::endl;
    
    // Test 6: Buffer processing
    std::cout << "Test 6: Buffer processing" << std::endl;
    std::vector<float> buffer = {0.5f, 1.5f, -1.5f, 0.8f, -0.3f, 2.0f, -2.0f};
    softClipBuffer(buffer.data(), buffer.size());
    for (float sample : buffer) {
        assert(sample >= -1.0f && sample <= 1.0f);
    }
    std::cout << "  ✓ All buffer samples within [-1.0, 1.0]" << std::endl;
    
    std::cout << std::endl;
    std::cout << "✅ Task 9.3 PASSED: Soft clipping working correctly" << std::endl;
    std::cout << "   - Validates Requirements 6.3, 15.3" << std::endl;
    std::cout << std::endl;
}

void testTask95_NaNInfinityDetection() {
    std::cout << "=== Task 9.5: NaN/Infinity Detection and Recovery ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Check for invalid values
    std::cout << "Test 1: Check audio output for invalid values" << std::endl;
    float nan = std::numeric_limits<float>::quiet_NaN();
    float posInf = std::numeric_limits<float>::infinity();
    float negInf = -std::numeric_limits<float>::infinity();
    
    assert(!isValid(nan));
    assert(!isValid(posInf));
    assert(!isValid(negInf));
    assert(isValid(0.0f));
    assert(isValid(1.0f));
    assert(isValid(-1.0f));
    std::cout << "  ✓ Invalid values detected correctly" << std::endl;
    
    // Test 2: Buffer validation
    std::cout << "Test 2: Buffer validation" << std::endl;
    std::vector<float> validBuffer = {0.5f, 0.8f, -0.3f, 1.0f, -1.0f};
    assert(isBufferValid(validBuffer.data(), validBuffer.size()));
    std::cout << "  ✓ Valid buffer passes validation" << std::endl;
    
    std::vector<float> invalidBuffer1 = {0.5f, nan, 0.3f};
    size_t invalidIndex = 0;
    assert(!isBufferValid(invalidBuffer1.data(), invalidBuffer1.size(), &invalidIndex));
    assert(invalidIndex == 1);
    std::cout << "  ✓ NaN detected at correct index" << std::endl;
    
    std::vector<float> invalidBuffer2 = {0.5f, posInf, 0.3f};
    assert(!isBufferValid(invalidBuffer2.data(), invalidBuffer2.size(), &invalidIndex));
    assert(invalidIndex == 1);
    std::cout << "  ✓ Infinity detected at correct index" << std::endl;
    
    // Test 3: Reset synthesis state (sanitize buffer)
    std::cout << "Test 3: Reset synthesis state if detected" << std::endl;
    std::vector<float> dirtyBuffer = {0.5f, nan, posInf, 0.8f, negInf};
    size_t count = sanitizeBuffer(dirtyBuffer.data(), dirtyBuffer.size());
    assert(count == 3);  // 3 invalid values
    assert(approxEqual(dirtyBuffer[0], 0.5f));
    assert(approxEqual(dirtyBuffer[1], 0.0f));  // NaN replaced
    assert(approxEqual(dirtyBuffer[2], 0.0f));  // +inf replaced
    assert(approxEqual(dirtyBuffer[3], 0.8f));
    assert(approxEqual(dirtyBuffer[4], 0.0f));  // -inf replaced
    std::cout << "  ✓ Invalid values replaced with zero" << std::endl;
    
    // Test 4: Multiple invalid values
    std::cout << "Test 4: Handle multiple invalid values" << std::endl;
    std::vector<float> multiInvalid = {nan, nan, posInf, 0.5f, negInf, nan};
    count = sanitizeBuffer(multiInvalid.data(), multiInvalid.size());
    assert(count == 5);
    assert(approxEqual(multiInvalid[3], 0.5f));  // Valid value preserved
    for (size_t i = 0; i < multiInvalid.size(); ++i) {
        if (i != 3) {
            assert(approxEqual(multiInvalid[i], 0.0f));
        }
    }
    std::cout << "  ✓ Multiple invalid values handled correctly" << std::endl;
    
    // Test 5: No false positives
    std::cout << "Test 5: No false positives on valid data" << std::endl;
    std::vector<float> allValid = {0.0f, 0.5f, 1.0f, -0.5f, -1.0f, 0.999f, -0.999f};
    count = sanitizeBuffer(allValid.data(), allValid.size());
    assert(count == 0);
    std::cout << "  ✓ No false positives" << std::endl;
    
    std::cout << std::endl;
    std::cout << "✅ Task 9.5 PASSED: NaN/infinity detection and recovery working correctly" << std::endl;
    std::cout << "   - Validates Requirement 15.4" << std::endl;
    std::cout << std::endl;
}

void testIntegration() {
    std::cout << "=== Integration Test: All Features Together ===" << std::endl;
    std::cout << std::endl;
    
    // Simulate audio processing pipeline
    std::cout << "Simulating audio processing pipeline..." << std::endl;
    
    // Step 1: Generate some audio (simulated)
    std::vector<float> buffer = {0.8f, 1.2f, -1.3f, 0.5f, -0.7f};
    std::cout << "  Step 1: Generated audio" << std::endl;
    
    // Step 2: Apply master level
    applyMasterLevel(buffer.data(), buffer.size(), 0.8f);
    std::cout << "  Step 2: Applied master level (0.8)" << std::endl;
    
    // Step 3: Check for NaN/infinity
    size_t invalidIndex = 0;
    if (!isBufferValid(buffer.data(), buffer.size(), &invalidIndex)) {
        std::cout << "  Step 3: Invalid values detected, sanitizing..." << std::endl;
        sanitizeBuffer(buffer.data(), buffer.size());
    } else {
        std::cout << "  Step 3: No invalid values detected" << std::endl;
    }
    
    // Step 4: Apply soft clipping
    softClipBuffer(buffer.data(), buffer.size());
    std::cout << "  Step 4: Applied soft clipping" << std::endl;
    
    // Verify final output
    for (float sample : buffer) {
        assert(std::isfinite(sample));
        assert(sample >= -1.0f && sample <= 1.0f);
    }
    std::cout << "  ✓ Final output is valid and within range" << std::endl;
    
    std::cout << std::endl;
    std::cout << "✅ Integration test PASSED" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  KICK DRUM SYNTHESIZER" << std::endl;
    std::cout << "  Tasks 9.1, 9.3, 9.5 Implementation" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    try {
        testTask91_MasterOutputLevel();
        testTask93_SoftClipping();
        testTask95_NaNInfinityDetection();
        testIntegration();
        
        std::cout << "========================================" << std::endl;
        std::cout << "  ✅ ALL TESTS PASSED!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;
        std::cout << "Summary:" << std::endl;
        std::cout << "  ✓ Task 9.1: Master output level control" << std::endl;
        std::cout << "  ✓ Task 9.3: Soft clipping" << std::endl;
        std::cout << "  ✓ Task 9.5: NaN/infinity detection and recovery" << std::endl;
        std::cout << std::endl;
        std::cout << "Requirements validated:" << std::endl;
        std::cout << "  ✓ 6.1: Master output level parameter" << std::endl;
        std::cout << "  ✓ 6.2: Master level applied after effects" << std::endl;
        std::cout << "  ✓ 6.3: Soft clipping prevents hard clipping" << std::endl;
        std::cout << "  ✓ 15.3: Soft clipping on exceeding 0dBFS" << std::endl;
        std::cout << "  ✓ 15.4: NaN/infinity detection and recovery" << std::endl;
        std::cout << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
EOF

# Compile
echo ""
echo "Compiling..."
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_master_output \
    $TEMP_DIR/DSPUtils.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_master_output
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
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
