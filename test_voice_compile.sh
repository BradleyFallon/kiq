#!/bin/bash

# Simple compilation test for Voice
# This verifies the code compiles and basic functionality works

echo "=== Testing Voice Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
mkdir -p $TEMP_DIR/generators
mkdir -p $TEMP_DIR/modulation
mkdir -p $TEMP_DIR/envelopes

# Copy generators
cp src/audio_engine/generators/SineDriver.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/SineDriver.h $TEMP_DIR/generators/
cp src/audio_engine/generators/HarmonicMembrane.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/HarmonicMembrane.h $TEMP_DIR/generators/
cp src/audio_engine/generators/NoiseGenerator.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/NoiseGenerator.h $TEMP_DIR/generators/

# Copy modulation
cp src/audio_engine/modulation/RingModulator.cpp $TEMP_DIR/modulation/
cp src/audio_engine/modulation/RingModulator.h $TEMP_DIR/modulation/

# Copy envelopes
cp src/audio_engine/envelopes/DualPhaseEnvelope.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/DualPhaseEnvelope.h $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/PitchEnvelope.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/PitchEnvelope.h $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/EnvelopeCurves.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/EnvelopeCurves.h $TEMP_DIR/envelopes/

# Copy Voice
cp src/audio_engine/voice/Voice.cpp $TEMP_DIR/
cp src/audio_engine/voice/Voice.h $TEMP_DIR/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "Voice.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace KickDrum;

bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

void testBasicFunctionality() {
    Voice voice;
    const float sampleRate = 48000.0f;
    
    // Test initialization
    assert(!voice.isActive());
    voice.initialize(sampleRate);
    assert(!voice.isActive());  // Should not be active until triggered
    std::cout << "✓ Initialization works" << std::endl;
    
    // Test triggering
    voice.trigger(60, 0.8f);
    assert(voice.isActive());
    assert(voice.getNote() == 60);
    assert(voice.getAge() == 0);
    std::cout << "✓ Trigger works" << std::endl;
    
    // Test rendering produces output
    bool foundNonZero = false;
    for (int i = 0; i < 100; i++) {
        float sample = voice.renderSample();
        if (std::abs(sample) > 0.0001f) {
            foundNonZero = true;
            break;
        }
    }
    assert(foundNonZero);
    std::cout << "✓ Rendering produces non-zero output" << std::endl;
    
    // Test age increments
    Voice voice2;
    voice2.initialize(sampleRate);
    voice2.trigger(60, 0.8f);
    assert(voice2.getAge() == 0);
    voice2.renderSample();
    assert(voice2.getAge() == 1);
    voice2.renderSample();
    assert(voice2.getAge() == 2);
    std::cout << "✓ Age increments correctly" << std::endl;
    
    // Test parameter setters
    Voice voice3;
    voice3.initialize(sampleRate);
    voice3.setBasePitch(60.0f);
    assert(voice3.getBasePitch() == 60.0f);
    voice3.setSineLevel(0.7f);
    assert(voice3.getSineLevel() == 0.7f);
    voice3.setHarmonicLevel(0.4f);
    assert(voice3.getHarmonicLevel() == 0.4f);
    voice3.setNoiseLevel(0.3f);
    assert(voice3.getNoiseLevel() == 0.3f);
    std::cout << "✓ Parameter setters work" << std::endl;
    
    // Test velocity scaling
    Voice voiceFull, voiceHalf;
    voiceFull.initialize(sampleRate);
    voiceHalf.initialize(sampleRate);
    
    // Configure simple envelopes
    voiceFull.getAmplitudeEnvelope().setWarmUpDuration(0.0f);
    voiceFull.getAmplitudeEnvelope().setAttack(0.0f);
    voiceFull.getAmplitudeEnvelope().setDecay(0.1f);
    voiceFull.getAmplitudeEnvelope().setSustain(1.0f);
    voiceFull.getPitchEnvelope().setDepth(0.0f);
    
    voiceHalf.getAmplitudeEnvelope().setWarmUpDuration(0.0f);
    voiceHalf.getAmplitudeEnvelope().setAttack(0.0f);
    voiceHalf.getAmplitudeEnvelope().setDecay(0.1f);
    voiceHalf.getAmplitudeEnvelope().setSustain(1.0f);
    voiceHalf.getPitchEnvelope().setDepth(0.0f);
    
    voiceFull.trigger(60, 1.0f);
    voiceHalf.trigger(60, 0.5f);
    
    // Skip a few samples
    for (int i = 0; i < 10; i++) {
        voiceFull.renderSample();
        voiceHalf.renderSample();
    }
    
    float fullSample = voiceFull.renderSample();
    float halfSample = voiceHalf.renderSample();
    
    // Half velocity should produce approximately half amplitude
    assert(approxEqual(halfSample, fullSample * 0.5f, 0.15f));
    std::cout << "✓ Velocity scaling works" << std::endl;
    
    // Test output is finite
    Voice voice4;
    voice4.initialize(sampleRate);
    voice4.trigger(60, 0.8f);
    for (int i = 0; i < 1000; i++) {
        float sample = voice4.renderSample();
        assert(std::isfinite(sample));
    }
    std::cout << "✓ Output is always finite" << std::endl;
    
    // Test release
    Voice voice5;
    voice5.initialize(sampleRate);
    voice5.trigger(60, 0.8f);
    assert(voice5.isActive());
    voice5.release();
    assert(voice5.isActive());  // Still active during release
    std::cout << "✓ Release works" << std::endl;
    
    // Test inactive voice produces zero
    Voice voice6;
    voice6.initialize(sampleRate);
    assert(!voice6.isActive());
    float sample = voice6.renderSample();
    assert(sample == 0.0f);
    std::cout << "✓ Inactive voice produces zero" << std::endl;
}

int main() {
    std::cout << "Testing Voice implementation..." << std::endl;
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
g++ -std=c++17 \
    -I$TEMP_DIR \
    -I$TEMP_DIR/generators \
    -I$TEMP_DIR/modulation \
    -I$TEMP_DIR/envelopes \
    -o $TEMP_DIR/test_voice \
    $TEMP_DIR/generators/SineDriver.cpp \
    $TEMP_DIR/generators/HarmonicMembrane.cpp \
    $TEMP_DIR/generators/NoiseGenerator.cpp \
    $TEMP_DIR/modulation/RingModulator.cpp \
    $TEMP_DIR/envelopes/EnvelopeCurves.cpp \
    $TEMP_DIR/envelopes/DualPhaseEnvelope.cpp \
    $TEMP_DIR/envelopes/PitchEnvelope.cpp \
    $TEMP_DIR/Voice.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_voice
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== Voice implementation verified! ==="
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
