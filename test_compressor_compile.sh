#!/bin/bash

# Simple compilation test for Compressor
# This verifies the code compiles without CMake

echo "=== Testing Compressor Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/effects/Compressor.cpp $TEMP_DIR/
cp src/audio_engine/effects/Compressor.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "Compressor.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>

using namespace KickDrum;

// Helper function to convert dB to linear
float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

// Helper function to convert linear to dB
float linearToDb(float linear) {
    if (linear < 1e-10f) return -96.0f;
    return 20.0f * std::log10(linear);
}

// Helper function to calculate RMS
float calculateRMS(const std::vector<float>& samples) {
    float sum = 0.0f;
    for (float sample : samples) {
        sum += sample * sample;
    }
    return std::sqrt(sum / samples.size());
}

void testInitialization() {
    std::cout << "Testing initialization..." << std::endl;
    
    Compressor comp;
    assert(!comp.isInitialized());
    
    comp.initialize(48000.0f);
    assert(comp.isInitialized());
    
    std::cout << "✓ Initialization works" << std::endl;
}

void testDefaultParameters() {
    std::cout << "Testing default parameters..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    
    assert(comp.getThreshold() == -12.0f);
    assert(comp.getRatio() == 4.0f);
    assert(comp.getAttack() == 0.005f);
    assert(comp.getRelease() == 0.1f);
    assert(comp.getMix() == 1.0f);
    
    std::cout << "✓ Default parameters correct" << std::endl;
}

void testParameterSettersGetters() {
    std::cout << "Testing parameter setters/getters..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    
    comp.setThreshold(-20.0f);
    assert(comp.getThreshold() == -20.0f);
    
    comp.setRatio(8.0f);
    assert(comp.getRatio() == 8.0f);
    
    comp.setAttack(0.01f);
    assert(comp.getAttack() == 0.01f);
    
    comp.setRelease(0.2f);
    assert(comp.getRelease() == 0.2f);
    
    comp.setMix(0.5f);
    assert(comp.getMix() == 0.5f);
    
    std::cout << "✓ Parameter setters/getters work" << std::endl;
}

void testParameterClamping() {
    std::cout << "Testing parameter clamping..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    
    // Threshold clamping
    comp.setThreshold(-100.0f);
    assert(comp.getThreshold() == -60.0f);
    comp.setThreshold(10.0f);
    assert(comp.getThreshold() == 0.0f);
    
    // Ratio clamping
    comp.setRatio(0.5f);
    assert(comp.getRatio() == 1.0f);
    comp.setRatio(50.0f);
    assert(comp.getRatio() == 20.0f);
    
    // Attack clamping
    comp.setAttack(0.00001f);
    assert(comp.getAttack() == 0.0001f);
    comp.setAttack(1.0f);
    assert(comp.getAttack() == 0.1f);
    
    // Release clamping
    comp.setRelease(0.001f);
    assert(comp.getRelease() == 0.01f);
    comp.setRelease(10.0f);
    assert(comp.getRelease() == 1.0f);
    
    // Mix clamping
    comp.setMix(-0.5f);
    assert(comp.getMix() == 0.0f);
    comp.setMix(2.0f);
    assert(comp.getMix() == 1.0f);
    
    std::cout << "✓ Parameter clamping works" << std::endl;
}

void testBypassWhenNotInitialized() {
    std::cout << "Testing bypass when not initialized..." << std::endl;
    
    Compressor comp;
    float input = 0.5f;
    float output = comp.process(input);
    assert(output == input);
    
    std::cout << "✓ Bypass when not initialized works" << std::endl;
}

void testNoCompressionBelowThreshold() {
    std::cout << "Testing no compression below threshold..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    comp.setThreshold(-20.0f);
    comp.setRatio(4.0f);
    comp.setMix(1.0f);
    
    // Generate a quiet signal (-30dB, below threshold)
    float amplitude = dbToLinear(-30.0f);
    std::vector<float> input;
    std::vector<float> output;
    
    for (int i = 0; i < 1000; ++i) {
        float phase = 2.0f * M_PI * 100.0f * i / 48000.0f;
        float sample = amplitude * std::sin(phase);
        input.push_back(sample);
        output.push_back(comp.process(sample));
    }
    
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // Output should be very close to input
    assert(std::abs(outputRMS - inputRMS) < 0.01f);
    
    std::cout << "✓ No compression below threshold" << std::endl;
}

void testCompressionAboveThreshold() {
    std::cout << "Testing compression above threshold..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    comp.setThreshold(-20.0f);
    comp.setRatio(4.0f);
    comp.setMix(1.0f);
    comp.setAttack(0.001f);
    comp.setRelease(0.05f);
    
    // Generate a loud signal (-10dB, above threshold)
    float amplitude = dbToLinear(-10.0f);
    std::vector<float> input;
    std::vector<float> output;
    
    for (int i = 0; i < 5000; ++i) {
        float phase = 2.0f * M_PI * 100.0f * i / 48000.0f;
        float sample = amplitude * std::sin(phase);
        input.push_back(sample);
        output.push_back(comp.process(sample));
    }
    
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // Output should be quieter than input
    assert(outputRMS < inputRMS);
    
    std::cout << "✓ Compression above threshold works" << std::endl;
}

void testRatioOneNoCompression() {
    std::cout << "Testing ratio 1.0 (no compression)..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    comp.setThreshold(-20.0f);
    comp.setRatio(1.0f);
    comp.setMix(1.0f);
    
    // Generate a loud signal
    float amplitude = dbToLinear(-10.0f);
    std::vector<float> input;
    std::vector<float> output;
    
    for (int i = 0; i < 1000; ++i) {
        float phase = 2.0f * M_PI * 100.0f * i / 48000.0f;
        float sample = amplitude * std::sin(phase);
        input.push_back(sample);
        output.push_back(comp.process(sample));
    }
    
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // With ratio = 1.0, no compression should occur
    assert(std::abs(outputRMS - inputRMS) < 0.01f);
    
    std::cout << "✓ Ratio 1.0 produces no compression" << std::endl;
}

void testFullyDryMix() {
    std::cout << "Testing fully dry mix..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    comp.setThreshold(-20.0f);
    comp.setRatio(4.0f);
    comp.setMix(0.0f);  // Fully dry
    
    // Generate a loud signal
    float amplitude = dbToLinear(-10.0f);
    std::vector<float> input;
    std::vector<float> output;
    
    for (int i = 0; i < 1000; ++i) {
        float phase = 2.0f * M_PI * 100.0f * i / 48000.0f;
        float sample = amplitude * std::sin(phase);
        input.push_back(sample);
        output.push_back(comp.process(sample));
    }
    
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // With mix = 0.0, output should equal input
    assert(std::abs(outputRMS - inputRMS) < 0.001f);
    
    std::cout << "✓ Fully dry mix passes signal through" << std::endl;
}

void testReset() {
    std::cout << "Testing reset..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    comp.setThreshold(-20.0f);
    comp.setRatio(4.0f);
    comp.setMix(1.0f);
    
    // Build up gain reduction
    float amplitude = dbToLinear(-10.0f);
    for (int i = 0; i < 1000; ++i) {
        comp.process(amplitude);
    }
    
    assert(comp.getGainReduction() > 0.0f);
    
    comp.reset();
    assert(comp.getGainReduction() == 0.0f);
    
    std::cout << "✓ Reset clears gain reduction" << std::endl;
}

void testZeroInputHandling() {
    std::cout << "Testing zero input handling..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    comp.setThreshold(-20.0f);
    comp.setRatio(4.0f);
    comp.setMix(1.0f);
    
    float output = comp.process(0.0f);
    
    assert(!std::isnan(output));
    assert(!std::isinf(output));
    assert(output == 0.0f);
    
    std::cout << "✓ Zero input handled correctly" << std::endl;
}

void testNegativeInputHandling() {
    std::cout << "Testing negative input handling..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    comp.setThreshold(-20.0f);
    comp.setRatio(4.0f);
    comp.setMix(1.0f);
    
    float input = -0.5f;
    float output = comp.process(input);
    
    assert(!std::isnan(output));
    assert(!std::isinf(output));
    assert(output < 0.0f);
    
    std::cout << "✓ Negative input handled correctly" << std::endl;
}

void testGainReductionNonNegative() {
    std::cout << "Testing gain reduction is non-negative..." << std::endl;
    
    Compressor comp;
    comp.initialize(48000.0f);
    comp.setThreshold(-20.0f);
    comp.setRatio(4.0f);
    comp.setMix(1.0f);
    
    std::vector<float> testAmplitudes = {0.0f, 0.1f, 0.5f, 1.0f};
    
    for (float amplitude : testAmplitudes) {
        comp.reset();
        for (int i = 0; i < 100; ++i) {
            comp.process(amplitude);
        }
        assert(comp.getGainReduction() >= 0.0f);
    }
    
    std::cout << "✓ Gain reduction is always non-negative" << std::endl;
}

int main() {
    std::cout << "Testing Compressor implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testInitialization();
        testDefaultParameters();
        testParameterSettersGetters();
        testParameterClamping();
        testBypassWhenNotInitialized();
        testNoCompressionBelowThreshold();
        testCompressionAboveThreshold();
        testRatioOneNoCompression();
        testFullyDryMix();
        testReset();
        testZeroInputHandling();
        testNegativeInputHandling();
        testGainReductionNonNegative();
        
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_compressor \
    $TEMP_DIR/Compressor.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_compressor
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== Compressor implementation verified! ==="
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
