#!/bin/bash

# Simple compilation test for PitchEnvelope
# This verifies the code compiles and basic functionality works

echo "=== Testing PitchEnvelope Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temporary directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/envelopes/EnvelopeCurves.h $TEMP_DIR/
cp src/audio_engine/envelopes/EnvelopeCurves.cpp $TEMP_DIR/
cp src/audio_engine/envelopes/DualPhaseEnvelope.h $TEMP_DIR/
cp src/audio_engine/envelopes/DualPhaseEnvelope.cpp $TEMP_DIR/
cp src/audio_engine/envelopes/PitchEnvelope.h $TEMP_DIR/
cp src/audio_engine/envelopes/PitchEnvelope.cpp $TEMP_DIR/

# Create a simple test main file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include <iostream>
#include <cmath>
#include <cassert>
#include "PitchEnvelope.h"

using namespace KickDrum;

// Helper to check approximate equality
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void testDepthParameter() {
    std::cout << "Testing depth parameter..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    
    // Test default value
    assert(pitchEnv.getDepth() == 500.0f);
    
    // Test setter
    pitchEnv.setDepth(1000.0f);
    assert(pitchEnv.getDepth() == 1000.0f);
    
    // Test clamping to minimum
    pitchEnv.setDepth(-100.0f);
    assert(pitchEnv.getDepth() == 0.0f);
    
    // Test clamping to maximum
    pitchEnv.setDepth(3000.0f);
    assert(pitchEnv.getDepth() == 2000.0f);
    
    std::cout << "  ✓ Depth parameter works correctly" << std::endl;
}

void testInitialState() {
    std::cout << "Testing initial state..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    
    // Before triggering, should be inactive with value 0
    assert(!pitchEnv.isActive());
    assert(pitchEnv.getValue() == 0.0f);
    
    std::cout << "  ✓ Initial state is correct" << std::endl;
}

void testZeroDepth() {
    std::cout << "Testing zero depth..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    pitchEnv.setDepth(0.0f);
    pitchEnv.trigger();
    
    // Advance a bit
    for (int i = 0; i < 480; ++i) {
        pitchEnv.advance();
    }
    
    // Value should always be 0 when depth is 0
    assert(pitchEnv.getValue() == 0.0f);
    
    std::cout << "  ✓ Zero depth produces zero offset" << std::endl;
}

void testValueScalesWithDepth() {
    std::cout << "Testing value scaling with depth..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    pitchEnv.setDepth(1000.0f);
    
    pitchEnv.trigger();
    
    // Advance to peak (after attack phase, 2ms = 96 samples)
    for (int i = 0; i < 96; ++i) {
        pitchEnv.advance();
    }
    
    // Value should be close to depth (1000Hz) at peak
    float value = pitchEnv.getValue();
    assert(value > 900.0f);
    assert(value <= 1000.0f);
    
    std::cout << "  ✓ Value scales correctly with depth" << std::endl;
}

void testValueDecaysToZero() {
    std::cout << "Testing value decay to zero..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    pitchEnv.setDepth(500.0f);
    pitchEnv.trigger();
    
    // Advance through entire envelope (200ms = 9600 samples)
    for (int i = 0; i < 9600; ++i) {
        pitchEnv.advance();
    }
    
    // Value should be at or near 0
    assert(pitchEnv.getValue() < 10.0f);
    
    std::cout << "  ✓ Value decays to zero" << std::endl;
}

void testTriggerActivatesEnvelope() {
    std::cout << "Testing trigger activation..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    assert(!pitchEnv.isActive());
    
    pitchEnv.trigger();
    
    assert(pitchEnv.isActive());
    
    std::cout << "  ✓ Trigger activates envelope" << std::endl;
}

void testResetDeactivatesEnvelope() {
    std::cout << "Testing reset deactivation..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    pitchEnv.trigger();
    assert(pitchEnv.isActive());
    
    pitchEnv.reset();
    
    assert(!pitchEnv.isActive());
    assert(pitchEnv.getValue() == 0.0f);
    
    std::cout << "  ✓ Reset deactivates envelope" << std::endl;
}

void testEnvelopeAccess() {
    std::cout << "Testing envelope access..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    
    // Access underlying envelope
    DualPhaseEnvelope& envelope = pitchEnv.getEnvelope();
    
    // Modify parameters
    envelope.setAttack(0.05f);
    
    // Verify change
    assert(envelope.getAttack() == 0.05f);
    
    std::cout << "  ✓ Can access and modify underlying envelope" << std::endl;
}

void testTypicalKickDrumPitchSweep() {
    std::cout << "Testing typical kick drum pitch sweep..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    pitchEnv.setDepth(800.0f);
    pitchEnv.getEnvelope().setAttack(0.001f);
    pitchEnv.getEnvelope().setDecay(0.08f);
    pitchEnv.getEnvelope().setSustain(0.0f);
    pitchEnv.getEnvelope().setRelease(0.02f);
    
    pitchEnv.trigger();
    
    // At start, should be near 0
    assert(pitchEnv.getValue() < 50.0f);
    
    // After attack (2ms = 96 samples), should be near peak
    for (int i = 0; i < 96; ++i) {
        pitchEnv.advance();
    }
    float peakValue = pitchEnv.getValue();
    assert(peakValue > 700.0f);
    assert(peakValue <= 800.0f);
    
    // After 40ms (1920 samples total), should be decaying
    for (int i = 0; i < 1824; ++i) {
        pitchEnv.advance();
    }
    float midValue = pitchEnv.getValue();
    assert(midValue > 100.0f);
    assert(midValue < 700.0f);
    
    // After full envelope (150ms = 7200 samples total), should be near 0
    for (int i = 0; i < 5280; ++i) {
        pitchEnv.advance();
    }
    assert(pitchEnv.getValue() < 50.0f);
    
    std::cout << "  ✓ Typical kick drum pitch sweep works correctly" << std::endl;
}

void testRetrigger() {
    std::cout << "Testing retrigger behavior..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    pitchEnv.setDepth(500.0f);
    
    // First trigger
    pitchEnv.trigger();
    for (int i = 0; i < 2400; ++i) { // 50ms
        pitchEnv.advance();
    }
    
    float firstValue = pitchEnv.getValue();
    assert(firstValue > 0.0f);
    
    // Retrigger
    pitchEnv.trigger();
    
    // Should restart from beginning
    float retriggeredValue = pitchEnv.getValue();
    assert(retriggeredValue < firstValue);
    
    // After advancing, should reach peak again
    for (int i = 0; i < 96; ++i) { // 2ms
        pitchEnv.advance();
    }
    float newPeakValue = pitchEnv.getValue();
    assert(newPeakValue > 400.0f);
    
    std::cout << "  ✓ Retrigger works correctly" << std::endl;
}

void testMaximumDepth() {
    std::cout << "Testing maximum depth..." << std::endl;
    
    PitchEnvelope pitchEnv(48000.0f);
    pitchEnv.setDepth(2000.0f);
    pitchEnv.trigger();
    
    // Advance to peak
    for (int i = 0; i < 96; ++i) {
        pitchEnv.advance();
    }
    
    float value = pitchEnv.getValue();
    assert(value > 1800.0f);
    assert(value <= 2000.0f);
    
    std::cout << "  ✓ Maximum depth works correctly" << std::endl;
}

int main() {
    std::cout << "Testing PitchEnvelope implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testDepthParameter();
        testInitialState();
        testZeroDepth();
        testValueScalesWithDepth();
        testValueDecaysToZero();
        testTriggerActivatesEnvelope();
        testResetDeactivatesEnvelope();
        testEnvelopeAccess();
        testTypicalKickDrumPitchSweep();
        testRetrigger();
        testMaximumDepth();
        
        std::cout << std::endl;
        std::cout << "✓ All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
EOF

echo ""
echo "Compiling..."
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_pitch_envelope \
    $TEMP_DIR/EnvelopeCurves.cpp \
    $TEMP_DIR/DualPhaseEnvelope.cpp \
    $TEMP_DIR/PitchEnvelope.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_pitch_envelope
    TEST_RESULT=$?
    echo ""
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✓ All tests passed!"
    else
        echo "✗ Tests failed with exit code $TEST_RESULT"
    fi
    
    # Clean up
    rm -rf $TEMP_DIR
    exit $TEST_RESULT
else
    echo "✗ Compilation failed!"
    rm -rf $TEMP_DIR
    exit 1
fi
