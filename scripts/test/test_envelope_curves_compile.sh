#!/bin/bash

# Simple compilation test for EnvelopeCurves
# This verifies the code compiles and basic functionality works

echo "=== Testing EnvelopeCurves Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/envelopes/EnvelopeCurves.cpp $TEMP_DIR/
cp src/audio_engine/envelopes/EnvelopeCurves.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "EnvelopeCurves.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace KickDrum;

bool approxEqual(float a, float b, float epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}

void testLinearCurve() {
    std::cout << "Testing Linear Curve..." << std::endl;
    
    // Linear curve should be identity function
    assert(applyLinearCurve(0.0f) == 0.0f);
    assert(applyLinearCurve(0.5f) == 0.5f);
    assert(applyLinearCurve(1.0f) == 1.0f);
    
    std::cout << "✓ Linear curve works correctly" << std::endl;
}

void testExponentialCurve() {
    std::cout << "Testing Exponential Curve..." << std::endl;
    
    // Exponential curve should be t^2
    assert(applyExponentialCurve(0.0f) == 0.0f);
    assert(applyExponentialCurve(0.5f) == 0.25f);
    assert(applyExponentialCurve(1.0f) == 1.0f);
    
    // Should be below linear for t in (0, 1)
    for (float t = 0.1f; t < 1.0f; t += 0.1f) {
        assert(applyExponentialCurve(t) < t);
    }
    
    std::cout << "✓ Exponential curve works correctly" << std::endl;
}

void testLogarithmicCurve() {
    std::cout << "Testing Logarithmic Curve..." << std::endl;
    
    // Logarithmic curve should be sqrt(t)
    assert(applyLogarithmicCurve(0.0f) == 0.0f);
    assert(approxEqual(applyLogarithmicCurve(0.5f), 0.707107f));
    assert(applyLogarithmicCurve(1.0f) == 1.0f);
    
    // Should be above linear for t in (0, 1)
    for (float t = 0.1f; t < 1.0f; t += 0.1f) {
        assert(applyLogarithmicCurve(t) > t);
    }
    
    std::cout << "✓ Logarithmic curve works correctly" << std::endl;
}

void testCustomCurve() {
    std::cout << "Testing Custom Curve..." << std::endl;
    
    // Custom curve should be t^3
    assert(applyCustomCurve(0.0f) == 0.0f);
    assert(applyCustomCurve(0.5f) == 0.125f);
    assert(applyCustomCurve(1.0f) == 1.0f);
    
    std::cout << "✓ Custom curve works correctly" << std::endl;
}

void testApplyCurveDispatcher() {
    std::cout << "Testing applyCurve() dispatcher..." << std::endl;
    
    float t = 0.5f;
    
    // Test each curve type
    assert(applyCurve(t, CurveType::LINEAR) == applyLinearCurve(t));
    assert(applyCurve(t, CurveType::EXPONENTIAL) == applyExponentialCurve(t));
    assert(applyCurve(t, CurveType::LOGARITHMIC) == applyLogarithmicCurve(t));
    assert(applyCurve(t, CurveType::CUSTOM) == applyCustomCurve(t));
    
    std::cout << "✓ applyCurve() dispatcher works correctly" << std::endl;
}

void testInputClamping() {
    std::cout << "Testing input clamping..." << std::endl;
    
    // Values below 0 should be clamped to 0
    assert(applyCurve(-0.5f, CurveType::LINEAR) == 0.0f);
    assert(applyCurve(-1.0f, CurveType::EXPONENTIAL) == 0.0f);
    
    // Values above 1 should be clamped to 1
    assert(applyCurve(1.5f, CurveType::LINEAR) == 1.0f);
    assert(applyCurve(2.0f, CurveType::EXPONENTIAL) == 1.0f);
    
    std::cout << "✓ Input clamping works correctly" << std::endl;
}

void testCurveOrdering() {
    std::cout << "Testing curve ordering..." << std::endl;
    
    // At t=0.5, curves should be ordered:
    // Custom (0.125) < Exponential (0.25) < Linear (0.5) < Logarithmic (0.707)
    float t = 0.5f;
    float custom = applyCustomCurve(t);
    float exponential = applyExponentialCurve(t);
    float linear = applyLinearCurve(t);
    float logarithmic = applyLogarithmicCurve(t);
    
    assert(custom < exponential);
    assert(exponential < linear);
    assert(linear < logarithmic);
    
    std::cout << "✓ Curve ordering is correct" << std::endl;
}

void testBoundaryBehavior() {
    std::cout << "Testing boundary behavior..." << std::endl;
    
    // All curves should start at 0
    assert(applyCurve(0.0f, CurveType::LINEAR) == 0.0f);
    assert(applyCurve(0.0f, CurveType::EXPONENTIAL) == 0.0f);
    assert(applyCurve(0.0f, CurveType::LOGARITHMIC) == 0.0f);
    assert(applyCurve(0.0f, CurveType::CUSTOM) == 0.0f);
    
    // All curves should end at 1
    assert(applyCurve(1.0f, CurveType::LINEAR) == 1.0f);
    assert(applyCurve(1.0f, CurveType::EXPONENTIAL) == 1.0f);
    assert(applyCurve(1.0f, CurveType::LOGARITHMIC) == 1.0f);
    assert(applyCurve(1.0f, CurveType::CUSTOM) == 1.0f);
    
    std::cout << "✓ Boundary behavior is correct" << std::endl;
}

void testValidRange() {
    std::cout << "Testing valid range..." << std::endl;
    
    // All curves should output values in [0.0, 1.0] for input in [0.0, 1.0]
    for (float t = 0.0f; t <= 1.0f; t += 0.05f) {
        float linear = applyCurve(t, CurveType::LINEAR);
        float exponential = applyCurve(t, CurveType::EXPONENTIAL);
        float logarithmic = applyCurve(t, CurveType::LOGARITHMIC);
        float custom = applyCurve(t, CurveType::CUSTOM);
        
        assert(linear >= 0.0f && linear <= 1.0f);
        assert(exponential >= 0.0f && exponential <= 1.0f);
        assert(logarithmic >= 0.0f && logarithmic <= 1.0f);
        assert(custom >= 0.0f && custom <= 1.0f);
    }
    
    std::cout << "✓ All curves stay within valid range [0.0, 1.0]" << std::endl;
}

void testMonotonicity() {
    std::cout << "Testing monotonicity..." << std::endl;
    
    // All curves should be monotonically increasing
    CurveType types[] = {CurveType::LINEAR, CurveType::EXPONENTIAL, 
                         CurveType::LOGARITHMIC, CurveType::CUSTOM};
    
    for (auto type : types) {
        float prev = applyCurve(0.0f, type);
        for (float t = 0.1f; t <= 1.0f; t += 0.1f) {
            float current = applyCurve(t, type);
            assert(current >= prev);
            prev = current;
        }
    }
    
    std::cout << "✓ All curves are monotonically increasing" << std::endl;
}

int main() {
    std::cout << "Testing EnvelopeCurves implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testLinearCurve();
        testExponentialCurve();
        testLogarithmicCurve();
        testCustomCurve();
        testApplyCurveDispatcher();
        testInputClamping();
        testCurveOrdering();
        testBoundaryBehavior();
        testValidRange();
        testMonotonicity();
        
        std::cout << std::endl;
        std::cout << "=== All tests passed! ===" << std::endl;
        std::cout << std::endl;
        std::cout << "Summary:" << std::endl;
        std::cout << "  ✓ Linear curve (identity function)" << std::endl;
        std::cout << "  ✓ Exponential curve (t^2 - accelerating)" << std::endl;
        std::cout << "  ✓ Logarithmic curve (sqrt(t) - decelerating)" << std::endl;
        std::cout << "  ✓ Custom curve (t^3 - more pronounced acceleration)" << std::endl;
        std::cout << "  ✓ Input clamping to [0.0, 1.0]" << std::endl;
        std::cout << "  ✓ All curves start at 0 and end at 1" << std::endl;
        std::cout << "  ✓ All curves are monotonically increasing" << std::endl;
        std::cout << "  ✓ All curves stay within valid range" << std::endl;
        
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_envelope_curves \
    $TEMP_DIR/EnvelopeCurves.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_envelope_curves
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== EnvelopeCurves implementation verified! ==="
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
