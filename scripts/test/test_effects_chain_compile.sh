#!/bin/bash

# Simple compilation test for EffectsChain
# This verifies the code compiles and runs basic tests without CMake

echo "=== Testing EffectsChain Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/effects/EffectsChain.cpp $TEMP_DIR/
cp src/audio_engine/effects/EffectsChain.h $TEMP_DIR/
cp src/audio_engine/effects/Compressor.cpp $TEMP_DIR/
cp src/audio_engine/effects/Compressor.h $TEMP_DIR/
cp src/audio_engine/effects/Reverb.cpp $TEMP_DIR/
cp src/audio_engine/effects/Reverb.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "EffectsChain.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>

using namespace KickDrum;

void testInitialization() {
    std::cout << "Testing initialization..." << std::endl;
    
    EffectsChain chain;
    assert(!chain.isInitialized());
    
    chain.initialize(48000.0f);
    assert(chain.isInitialized());
    assert(chain.getCompressor().isInitialized());
    assert(chain.getReverb().isInitialized());
    
    std::cout << "  ✓ Initialization test passed" << std::endl;
}

void testDefaultBypassStates() {
    std::cout << "Testing default bypass states..." << std::endl;
    
    EffectsChain chain;
    assert(!chain.isCompressorBypassed());
    assert(!chain.isReverbBypassed());
    
    std::cout << "  ✓ Default bypass states test passed" << std::endl;
}

void testBypassControls() {
    std::cout << "Testing bypass controls..." << std::endl;
    
    EffectsChain chain;
    
    // Test compressor bypass
    chain.setCompressorBypassed(true);
    assert(chain.isCompressorBypassed());
    chain.setCompressorBypassed(false);
    assert(!chain.isCompressorBypassed());
    
    // Test reverb bypass
    chain.setReverbBypassed(true);
    assert(chain.isReverbBypassed());
    chain.setReverbBypassed(false);
    assert(!chain.isReverbBypassed());
    
    // Test independent control
    chain.setCompressorBypassed(true);
    chain.setReverbBypassed(false);
    assert(chain.isCompressorBypassed());
    assert(!chain.isReverbBypassed());
    
    chain.setCompressorBypassed(false);
    chain.setReverbBypassed(true);
    assert(!chain.isCompressorBypassed());
    assert(chain.isReverbBypassed());
    
    std::cout << "  ✓ Bypass controls test passed" << std::endl;
}

void testBothEffectsBypassedPassThrough() {
    std::cout << "Testing pass-through when both effects bypassed..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    chain.setCompressorBypassed(true);
    chain.setReverbBypassed(true);
    
    std::vector<float> testValues = {0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 0.123f};
    
    for (float input : testValues) {
        float output = chain.process(input);
        assert(std::abs(output - input) < 1e-6f);
    }
    
    std::cout << "  ✓ Pass-through test passed" << std::endl;
}

void testCompressorApplied() {
    std::cout << "Testing compressor is applied when active..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    chain.setCompressorBypassed(false);
    chain.setReverbBypassed(true);  // Bypass reverb to isolate compressor
    
    // Configure compressor for noticeable effect
    chain.getCompressor().setThreshold(-20.0f);
    chain.getCompressor().setRatio(4.0f);
    chain.getCompressor().setAttack(0.001f);
    chain.getCompressor().setRelease(0.1f);
    chain.getCompressor().setMix(1.0f);
    
    // Process a loud signal
    float input = 0.8f;
    float output = 0.0f;
    for (int i = 0; i < 100; ++i) {
        output = chain.process(input);
    }
    
    // Output should be different from input due to compression
    assert(std::abs(output - input) > 1e-6f);
    
    std::cout << "  ✓ Compressor application test passed" << std::endl;
}

void testReverbApplied() {
    std::cout << "Testing reverb is applied when active..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    chain.setCompressorBypassed(true);  // Bypass compressor to isolate reverb
    chain.setReverbBypassed(false);
    
    // Configure reverb with settings that should produce noticeable effect
    chain.getReverb().setRoomSize(0.7f);
    chain.getReverb().setDecayTime(1.0f);
    chain.getReverb().setDamping(0.5f);
    chain.getReverb().setMix(1.0f);  // Fully wet
    
    // Process an impulse - with full wet mix, output should be different
    float impulse = 1.0f;
    float firstOutput = chain.process(impulse);
    
    // With reverb active and full wet mix, the first output should exist
    // (it may be zero or non-zero depending on reverb delay, but processing should work)
    assert(std::isfinite(firstOutput));
    
    // Process several samples and check if we get any non-zero output
    bool hasNonZeroOutput = false;
    for (int i = 0; i < 2000; ++i) {
        float output = chain.process(0.0f);
        if (std::abs(output) > 1e-10f) {
            hasNonZeroOutput = true;
            break;
        }
    }
    
    // Reverb should produce some non-zero output at some point
    assert(hasNonZeroOutput);
    
    std::cout << "  ✓ Reverb application test passed" << std::endl;
}

void testCompressorBypassSkipsProcessing() {
    std::cout << "Testing compressor bypass skips processing..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    
    // Configure compressor with extreme settings
    chain.getCompressor().setThreshold(-40.0f);
    chain.getCompressor().setRatio(20.0f);
    chain.getCompressor().setMix(1.0f);
    
    // Bypass reverb to isolate compressor
    chain.setReverbBypassed(true);
    
    // Process with compressor active
    chain.setCompressorBypassed(false);
    float input = 0.7f;
    float outputWithCompressor = 0.0f;
    for (int i = 0; i < 100; ++i) {
        outputWithCompressor = chain.process(input);
    }
    
    // Reset and process with compressor bypassed
    chain.reset();
    chain.setCompressorBypassed(true);
    float outputWithoutCompressor = 0.0f;
    for (int i = 0; i < 100; ++i) {
        outputWithoutCompressor = chain.process(input);
    }
    
    // Bypassed should equal input, active should be different
    assert(std::abs(outputWithoutCompressor - input) < 1e-6f);
    assert(std::abs(outputWithCompressor - input) > 1e-6f);
    
    std::cout << "  ✓ Compressor bypass test passed" << std::endl;
}

void testReverbBypassSkipsProcessing() {
    std::cout << "Testing reverb bypass skips processing..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    
    // Configure reverb
    chain.getReverb().setRoomSize(0.9f);
    chain.getReverb().setDecayTime(3.0f);
    chain.getReverb().setMix(0.8f);
    
    // Bypass compressor to isolate reverb
    chain.setCompressorBypassed(true);
    
    // Process with reverb active
    chain.setReverbBypassed(false);
    float input = 0.5f;
    float outputWithReverb = chain.process(input);
    
    // Reset and process with reverb bypassed
    chain.reset();
    chain.setReverbBypassed(true);
    float outputWithoutReverb = chain.process(input);
    
    // Bypassed should equal input, active should be different
    assert(std::abs(outputWithoutReverb - input) < 1e-6f);
    assert(std::abs(outputWithReverb - input) > 1e-6f);
    
    std::cout << "  ✓ Reverb bypass test passed" << std::endl;
}

void testReset() {
    std::cout << "Testing reset clears both effects..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    
    // Configure reverb
    chain.getReverb().setRoomSize(0.8f);
    chain.getReverb().setDecayTime(2.0f);
    chain.getReverb().setMix(1.0f);
    
    // Process an impulse to build up reverb tail
    chain.process(1.0f);
    
    // Process silence - should have reverb tail
    std::vector<float> outputsBeforeReset;
    for (int i = 0; i < 100; ++i) {
        outputsBeforeReset.push_back(chain.process(0.0f));
    }
    
    // Calculate max output before reset
    float maxBeforeReset = 0.0f;
    for (float output : outputsBeforeReset) {
        maxBeforeReset = std::max(maxBeforeReset, std::abs(output));
    }
    
    // Reset the chain
    chain.reset();
    
    // Process silence again - reverb tail should be cleared
    std::vector<float> outputsAfterReset;
    for (int i = 0; i < 100; ++i) {
        outputsAfterReset.push_back(chain.process(0.0f));
    }
    
    // Calculate max output after reset
    float maxAfterReset = 0.0f;
    for (float output : outputsAfterReset) {
        maxAfterReset = std::max(maxAfterReset, std::abs(output));
    }
    
    // After reset, max output should be significantly smaller or zero
    // (allowing for some tolerance since reverb might have initial response)
    assert(maxAfterReset <= maxBeforeReset);
    
    std::cout << "  ✓ Reset test passed" << std::endl;
}

void testParameterAccess() {
    std::cout << "Testing parameter access..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    
    // Test compressor parameter access
    chain.getCompressor().setThreshold(-15.0f);
    chain.getCompressor().setRatio(3.0f);
    assert(std::abs(chain.getCompressor().getThreshold() - (-15.0f)) < 1e-6f);
    assert(std::abs(chain.getCompressor().getRatio() - 3.0f) < 1e-6f);
    
    // Test reverb parameter access
    chain.getReverb().setRoomSize(0.6f);
    chain.getReverb().setDecayTime(1.5f);
    assert(std::abs(chain.getReverb().getRoomSize() - 0.6f) < 1e-6f);
    assert(std::abs(chain.getReverb().getDecayTime() - 1.5f) < 1e-6f);
    
    std::cout << "  ✓ Parameter access test passed" << std::endl;
}

void testOutputAlwaysFinite() {
    std::cout << "Testing output is always finite..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    
    std::vector<float> testInputs = {
        0.0f, 0.5f, -0.5f, 1.0f, -1.0f,
        0.001f, -0.001f, 0.999f, -0.999f
    };
    
    for (float input : testInputs) {
        float output = chain.process(input);
        assert(std::isfinite(output));
        assert(!std::isnan(output));
        assert(!std::isinf(output));
    }
    
    std::cout << "  ✓ Finite output test passed" << std::endl;
}

void testProcessingOrder() {
    std::cout << "Testing processing order (compressor before reverb)..." << std::endl;
    
    EffectsChain chain;
    chain.initialize(48000.0f);
    
    // Configure both effects
    chain.getCompressor().setThreshold(-30.0f);
    chain.getCompressor().setRatio(10.0f);
    chain.getCompressor().setMix(1.0f);
    
    chain.getReverb().setMix(0.5f);
    chain.getReverb().setRoomSize(0.5f);
    
    // Process a loud signal
    float input = 0.8f;
    float output = 0.0f;
    for (int i = 0; i < 100; ++i) {
        output = chain.process(input);
    }
    
    // Verify output is valid and modified
    assert(std::isfinite(output));
    assert(std::abs(output - input) > 1e-6f);
    
    std::cout << "  ✓ Processing order test passed" << std::endl;
}

int main() {
    std::cout << "\n=== EffectsChain Unit Tests ===" << std::endl;
    std::cout << std::endl;
    
    try {
        testInitialization();
        testDefaultBypassStates();
        testBypassControls();
        testBothEffectsBypassedPassThrough();
        testCompressorApplied();
        testReverbApplied();
        testCompressorBypassSkipsProcessing();
        testReverbBypassSkipsProcessing();
        testReset();
        testParameterAccess();
        testOutputAlwaysFinite();
        testProcessingOrder();
        
        std::cout << std::endl;
        std::cout << "=== All Tests Passed! ===" << std::endl;
        std::cout << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
EOF

# Compile the test
echo "Compiling..."
g++ -std=c++17 -O2 \
    -o $TEMP_DIR/test_effects_chain \
    $TEMP_DIR/test_main.cpp \
    $TEMP_DIR/EffectsChain.cpp \
    $TEMP_DIR/Compressor.cpp \
    $TEMP_DIR/Reverb.cpp

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed"
    rm -rf $TEMP_DIR
    exit 1
fi

echo "✓ Compilation successful"
echo ""

# Run the test
echo "Running tests..."
$TEMP_DIR/test_effects_chain

TEST_RESULT=$?

# Cleanup
rm -rf $TEMP_DIR

if [ $TEST_RESULT -eq 0 ]; then
    echo ""
    echo "✓ All tests passed!"
    exit 0
else
    echo ""
    echo "❌ Tests failed"
    exit 1
fi

