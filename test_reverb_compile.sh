#!/bin/bash

# Simple compilation test for Reverb
# This verifies the code compiles without CMake

echo "=== Testing Reverb Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/effects/Reverb.cpp $TEMP_DIR/
cp src/audio_engine/effects/Reverb.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "Reverb.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>

using namespace KickDrum;

void testInitialization() {
    std::cout << "Testing initialization..." << std::endl;
    
    Reverb reverb;
    assert(!reverb.isInitialized());
    
    reverb.initialize(48000.0f);
    assert(reverb.isInitialized());
    
    std::cout << "✓ Initialization works" << std::endl;
}

void testDefaultParameters() {
    std::cout << "Testing default parameters..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    assert(reverb.getRoomSize() == 0.5f);
    assert(reverb.getDecayTime() == 1.0f);
    assert(reverb.getDamping() == 0.5f);
    assert(reverb.getMix() == 0.3f);
    
    std::cout << "✓ Default parameters correct" << std::endl;
}

void testParameterSettersGetters() {
    std::cout << "Testing parameter setters/getters..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    reverb.setRoomSize(0.7f);
    assert(reverb.getRoomSize() == 0.7f);
    
    reverb.setDecayTime(2.5f);
    assert(reverb.getDecayTime() == 2.5f);
    
    reverb.setDamping(0.8f);
    assert(reverb.getDamping() == 0.8f);
    
    reverb.setMix(0.6f);
    assert(reverb.getMix() == 0.6f);
    
    std::cout << "✓ Parameter setters/getters work" << std::endl;
}

void testParameterClamping() {
    std::cout << "Testing parameter clamping..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    // Room size clamping
    reverb.setRoomSize(-0.5f);
    assert(reverb.getRoomSize() == 0.0f);
    reverb.setRoomSize(1.5f);
    assert(reverb.getRoomSize() == 1.0f);
    
    // Decay time clamping
    reverb.setDecayTime(0.05f);
    assert(reverb.getDecayTime() == 0.1f);
    reverb.setDecayTime(15.0f);
    assert(reverb.getDecayTime() == 10.0f);
    
    // Damping clamping
    reverb.setDamping(-0.5f);
    assert(reverb.getDamping() == 0.0f);
    reverb.setDamping(1.5f);
    assert(reverb.getDamping() == 1.0f);
    
    // Mix clamping
    reverb.setMix(-0.5f);
    assert(reverb.getMix() == 0.0f);
    reverb.setMix(2.0f);
    assert(reverb.getMix() == 1.0f);
    
    std::cout << "✓ Parameter clamping works" << std::endl;
}

void testBypassWhenNotInitialized() {
    std::cout << "Testing bypass when not initialized..." << std::endl;
    
    Reverb reverb;
    float input = 0.5f;
    float output = reverb.process(input);
    assert(output == input);
    
    std::cout << "✓ Bypass when not initialized works" << std::endl;
}

void testProcessSilence() {
    std::cout << "Testing process silence..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    for (int i = 0; i < 1000; i++) {
        float output = reverb.process(0.0f);
        assert(output == 0.0f);
    }
    
    std::cout << "✓ Silence processing works" << std::endl;
}

void testDrySignalWithZeroMix() {
    std::cout << "Testing dry signal with zero mix..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(0.0f);
    
    float input = 0.5f;
    float output = reverb.process(input);
    
    assert(output == input);
    
    std::cout << "✓ Zero mix passes dry signal" << std::endl;
}

void testReverbTailExists() {
    std::cout << "Testing reverb tail exists..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(1.0f);
    reverb.setRoomSize(0.7f);  // Larger room for more obvious tail
    
    // Send impulse and collect several outputs
    std::vector<float> outputs;
    outputs.push_back(reverb.process(1.0f));
    
    // Process more samples
    for (int i = 0; i < 2000; i++) {
        outputs.push_back(reverb.process(0.0f));
    }
    
    // Check if any output is non-zero
    bool hasNonZeroOutput = false;
    float maxOutput = 0.0f;
    for (size_t i = 0; i < outputs.size(); i++) {
        float absOut = std::abs(outputs[i]);
        if (absOut > maxOutput) {
            maxOutput = absOut;
        }
        if (absOut > 0.0f) {
            hasNonZeroOutput = true;
        }
    }
    
    std::cout << "  Max output: " << maxOutput << std::endl;
    assert(hasNonZeroOutput);
    
    std::cout << "✓ Reverb tail exists" << std::endl;
}

void testReverbTailDecays() {
    std::cout << "Testing reverb tail decays..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(1.0f);
    reverb.setDecayTime(0.3f);  // Very short decay for clear test
    
    // Send impulse
    reverb.process(1.0f);
    
    // Collect outputs
    std::vector<float> outputs;
    for (int i = 0; i < 15000; i++) {
        outputs.push_back(reverb.process(0.0f));
    }
    
    // Find peak in middle section (after initial delay, before decay)
    float middlePeak = 0.0f;
    for (int i = 100; i < 3000; i++) {
        middlePeak = std::max(middlePeak, std::abs(outputs[i]));
    }
    
    // Find peak in late section (after significant decay)
    float latePeak = 0.0f;
    for (int i = 12000; i < 15000; i++) {
        latePeak = std::max(latePeak, std::abs(outputs[i]));
    }
    
    std::cout << "  Middle peak: " << middlePeak << ", Late peak: " << latePeak << std::endl;
    
    // Late peak should be significantly smaller (reverb has decayed)
    assert(middlePeak > 0.0f);  // Ensure we have some reverb
    assert(latePeak < middlePeak * 0.5f);
    
    std::cout << "✓ Reverb tail decays" << std::endl;
}

void testResetClearsTail() {
    std::cout << "Testing reset clears tail..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(1.0f);
    
    // Send impulse
    reverb.process(1.0f);
    
    // Build up tail
    for (int i = 0; i < 100; i++) {
        reverb.process(0.0f);
    }
    
    // Reset
    reverb.reset();
    
    // After reset, silence should produce silence
    for (int i = 0; i < 100; i++) {
        float output = reverb.process(0.0f);
        assert(output == 0.0f);
    }
    
    std::cout << "✓ Reset clears tail" << std::endl;
}

void testStability() {
    std::cout << "Testing stability..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(1.0f);
    reverb.setRoomSize(0.9f);
    
    bool stable = true;
    for (int i = 0; i < 10000; i++) {
        float input = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / 48000.0f);
        float output = reverb.process(input);
        
        if (std::isnan(output) || std::isinf(output) || std::abs(output) > 10.0f) {
            stable = false;
            break;
        }
    }
    
    assert(stable);
    
    std::cout << "✓ Reverb is stable" << std::endl;
}

void testNoNaNOrInfinity() {
    std::cout << "Testing no NaN or infinity..." << std::endl;
    
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    std::vector<float> testInputs = {0.0f, 1.0f, -1.0f, 0.999f, -0.999f};
    
    for (float input : testInputs) {
        float output = reverb.process(input);
        assert(!std::isnan(output));
        assert(!std::isinf(output));
    }
    
    std::cout << "✓ No NaN or infinity" << std::endl;
}

void testDifferentSampleRates() {
    std::cout << "Testing different sample rates..." << std::endl;
    
    std::vector<float> sampleRates = {44100.0f, 48000.0f, 88200.0f, 96000.0f};
    
    for (float sr : sampleRates) {
        Reverb reverb;
        reverb.initialize(sr);
        
        assert(reverb.isInitialized());
        
        float output = reverb.process(1.0f);
        assert(!std::isnan(output));
        assert(!std::isinf(output));
    }
    
    std::cout << "✓ Works at different sample rates" << std::endl;
}

int main() {
    std::cout << "Testing Reverb implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testInitialization();
        testDefaultParameters();
        testParameterSettersGetters();
        testParameterClamping();
        testBypassWhenNotInitialized();
        testProcessSilence();
        testDrySignalWithZeroMix();
        testReverbTailExists();
        testReverbTailDecays();
        testResetClearsTail();
        testStability();
        testNoNaNOrInfinity();
        testDifferentSampleRates();
        
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_reverb \
    $TEMP_DIR/Reverb.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_reverb
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== Reverb implementation verified! ==="
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
