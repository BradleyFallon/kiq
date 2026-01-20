#!/bin/bash

# Simple compilation test for DualPhaseEnvelope
# This verifies the code compiles and basic functionality works

echo "=== Testing DualPhaseEnvelope Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temporary directory: $TEMP_DIR"

# Copy source files
cp src/audio_engine/envelopes/EnvelopeCurves.h $TEMP_DIR/
cp src/audio_engine/envelopes/EnvelopeCurves.cpp $TEMP_DIR/
cp src/audio_engine/envelopes/DualPhaseEnvelope.h $TEMP_DIR/
cp src/audio_engine/envelopes/DualPhaseEnvelope.cpp $TEMP_DIR/

# Create a simple test main file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include <iostream>
#include <cmath>
#include <cassert>
#include "DualPhaseEnvelope.h"

using namespace KickDrum;

// Helper to check approximate equality
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void testWarmUpParameters() {
    std::cout << "Testing warm-up parameters..." << std::endl;
    
    DualPhaseEnvelope env(48000.0f);
    
    // Test default values
    assert(env.getWarmUpDuration() == 0.0f);
    assert(env.getWarmUpStartFrequency() == 10.0f);
    assert(env.getWarmUpAmplitude() == 0.5f);
    
    // Test setters
    env.setWarmUpDuration(0.02f);
    assert(env.getWarmUpDuration() == 0.02f);
    
    env.setWarmUpStartFrequency(20.0f);
    assert(env.getWarmUpStartFrequency() == 20.0f);
    
    env.setWarmUpAmplitude(0.8f);
    assert(env.getWarmUpAmplitude() == 0.8f);
    
    // Test clamping
    env.setWarmUpDuration(-0.01f);
    assert(env.getWarmUpDuration() == 0.0f);
    
    env.setWarmUpDuration(0.2f);
    assert(env.getWarmUpDuration() == 0.1f);
    
    env.setWarmUpStartFrequency(2.0f);
    assert(env.getWarmUpStartFrequency() == 5.0f);
    
    env.setWarmUpStartFrequency(100.0f);
    assert(env.getWarmUpStartFrequency() == 50.0f);
    
    env.setWarmUpAmplitude(-0.1f);
    assert(env.getWarmUpAmplitude() == 0.0f);
    
    env.setWarmUpAmplitude(1.5f);
    assert(env.getWarmUpAmplitude() == 1.0f);
    
    std::cout << "  ✓ Warm-up parameters work correctly" << std::endl;
}

void testWarmUpPhaseBypass() {
    std::cout << "Testing warm-up phase bypass..." << std::endl;
    
    DualPhaseEnvelope env(48000.0f);
    env.setWarmUpDuration(0.0f);
    
    env.trigger();
    
    // Should skip WARMUP and go directly to ATTACK
    assert(env.getCurrentPhase() == EnvelopePhase::ATTACK);
    
    std::cout << "  ✓ Warm-up phase bypasses when duration is 0" << std::endl;
}

void testWarmUpPhaseActive() {
    std::cout << "Testing warm-up phase activation..." << std::endl;
    
    DualPhaseEnvelope env(48000.0f);
    env.setWarmUpDuration(0.02f);
    
    env.trigger();
    
    // Should enter WARMUP phase
    assert(env.getCurrentPhase() == EnvelopePhase::WARMUP);
    assert(env.isActive());
    
    std::cout << "  ✓ Warm-up phase activates when duration > 0" << std::endl;
}

void testWarmUpPhaseProgression() {
    std::cout << "Testing warm-up phase progression..." << std::endl;
    
    DualPhaseEnvelope env(48000.0f);
    env.setWarmUpDuration(0.02f); // 20ms = 960 samples
    env.setWarmUpAmplitude(0.8f);
    
    env.trigger();
    
    // Value should start at 0
    assert(env.getValue() == 0.0f);
    
    // Advance to middle of warm-up (10ms = 480 samples at 48kHz)
    for (int i = 0; i < 480; ++i) {
        env.advance();
    }
    
    // Should be around 0.4 (half of 0.8)
    float midValue = env.getValue();
    assert(midValue > 0.3f && midValue < 0.5f);
    
    // Advance to just before end of warm-up (479 more samples = 959 total)
    for (int i = 0; i < 479; ++i) {
        env.advance();
    }
    
    // Should be near warm-up amplitude (959/960 * 0.8 ≈ 0.799)
    float endValue = env.getValue();
    assert(endValue > 0.79f && endValue < 0.81f);
    
    std::cout << "  ✓ Warm-up phase progresses correctly" << std::endl;
}

void testWarmUpToAttackTransition() {
    std::cout << "Testing warm-up to attack transition..." << std::endl;
    
    DualPhaseEnvelope env(48000.0f);
    env.setWarmUpDuration(0.02f);
    env.setAttack(0.01f);
    
    env.trigger();
    
    // Should be in WARMUP
    assert(env.getCurrentPhase() == EnvelopePhase::WARMUP);
    
    // Advance past warm-up duration (21ms = 1008 samples)
    for (int i = 0; i < 1008; ++i) {
        env.advance();
    }
    
    // Should have transitioned to ATTACK
    assert(env.getCurrentPhase() == EnvelopePhase::ATTACK);
    
    std::cout << "  ✓ Warm-up transitions to attack correctly" << std::endl;
}

void testPhaseContinuity() {
    std::cout << "Testing phase continuity..." << std::endl;
    
    DualPhaseEnvelope env(48000.0f);
    env.setWarmUpDuration(0.02f);
    env.setWarmUpAmplitude(0.5f);
    env.setAttack(0.01f);
    
    env.trigger();
    
    // Advance to end of warm-up (20ms = 960 samples)
    for (int i = 0; i < 960; ++i) {
        env.advance();
    }
    
    // Should be in ATTACK phase now (transition happens at sample 960)
    assert(env.getCurrentPhase() == EnvelopePhase::ATTACK);
    
    // Value should be 0 at start of attack phase (phase continuity)
    // Use small epsilon for floating point comparison
    assert(env.getValue() < 0.001f);
    
    std::cout << "  ✓ Phase continuity maintained at transition" << std::endl;
}

void testADSRPhases() {
    std::cout << "Testing ADSR phases..." << std::endl;
    
    DualPhaseEnvelope env(48000.0f);
    env.setWarmUpDuration(0.0f);
    env.setAttack(0.01f);
    env.setDecay(0.02f);
    env.setSustain(0.3f);
    env.setRelease(0.01f);
    
    env.trigger();
    
    // Should start in ATTACK
    assert(env.getCurrentPhase() == EnvelopePhase::ATTACK);
    
    // Advance past attack (11ms = 528 samples)
    for (int i = 0; i < 528; ++i) {
        env.advance();
    }
    
    // Should be in DECAY
    assert(env.getCurrentPhase() == EnvelopePhase::DECAY);
    
    // Advance past decay (21ms = 1008 samples)
    for (int i = 0; i < 1008; ++i) {
        env.advance();
    }
    
    // Should be in SUSTAIN
    assert(env.getCurrentPhase() == EnvelopePhase::SUSTAIN);
    
    // Call release
    env.release();
    
    // Should be in RELEASE
    assert(env.getCurrentPhase() == EnvelopePhase::RELEASE);
    
    std::cout << "  ✓ ADSR phases work correctly" << std::endl;
}

void testZeroSustainAutoRelease() {
    std::cout << "Testing zero sustain auto-release..." << std::endl;
    
    DualPhaseEnvelope env(48000.0f);
    env.setWarmUpDuration(0.0f);
    env.setAttack(0.01f);
    env.setDecay(0.02f);
    env.setSustain(0.0f);
    env.setRelease(0.01f);
    
    env.trigger();
    
    // Advance past attack and decay (31ms = 1488 samples)
    for (int i = 0; i < 1488; ++i) {
        env.advance();
    }
    
    // Should auto-transition to RELEASE when sustain is 0
    assert(env.getCurrentPhase() == EnvelopePhase::RELEASE);
    
    std::cout << "  ✓ Zero sustain auto-releases correctly" << std::endl;
}

int main() {
    std::cout << "Testing DualPhaseEnvelope implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testWarmUpParameters();
        testWarmUpPhaseBypass();
        testWarmUpPhaseActive();
        testWarmUpPhaseProgression();
        testWarmUpToAttackTransition();
        testPhaseContinuity();
        testADSRPhases();
        testZeroSustainAutoRelease();
        
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
g++ -std=c++17 -I$TEMP_DIR -o $TEMP_DIR/test_dual_phase_envelope \
    $TEMP_DIR/EnvelopeCurves.cpp \
    $TEMP_DIR/DualPhaseEnvelope.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_dual_phase_envelope
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
