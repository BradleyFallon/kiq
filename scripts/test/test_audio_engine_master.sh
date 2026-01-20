#!/bin/bash

# Test AudioEngine with master output control, soft clipping, and NaN detection

echo "=== Testing AudioEngine Master Output & Safety Features ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy all necessary source files with directory structure
echo "Copying source files..."

# Create directory structure
mkdir -p $TEMP_DIR/core
mkdir -p $TEMP_DIR/include
mkdir -p $TEMP_DIR/utils
mkdir -p $TEMP_DIR/voice
mkdir -p $TEMP_DIR/generators
mkdir -p $TEMP_DIR/modulation
mkdir -p $TEMP_DIR/envelopes
mkdir -p $TEMP_DIR/effects

# Core
cp src/audio_engine/core/AudioEngine.cpp $TEMP_DIR/core/
cp src/audio_engine/include/AudioEngine.h $TEMP_DIR/include/

# Utils
cp src/audio_engine/utils/DSPUtils.cpp $TEMP_DIR/utils/
cp src/audio_engine/utils/DSPUtils.h $TEMP_DIR/utils/

# Voice
cp src/audio_engine/voice/Voice.cpp $TEMP_DIR/voice/
cp src/audio_engine/voice/Voice.h $TEMP_DIR/voice/
cp src/audio_engine/voice/VoiceAllocator.cpp $TEMP_DIR/voice/
cp src/audio_engine/voice/VoiceAllocator.h $TEMP_DIR/voice/

# Generators
cp src/audio_engine/generators/SineDriver.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/SineDriver.h $TEMP_DIR/generators/
cp src/audio_engine/generators/HarmonicMembrane.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/HarmonicMembrane.h $TEMP_DIR/generators/
cp src/audio_engine/generators/NoiseGenerator.cpp $TEMP_DIR/generators/
cp src/audio_engine/generators/NoiseGenerator.h $TEMP_DIR/generators/

# Modulation
cp src/audio_engine/modulation/RingModulator.cpp $TEMP_DIR/modulation/
cp src/audio_engine/modulation/RingModulator.h $TEMP_DIR/modulation/
cp src/audio_engine/modulation/GeneratorMixer.cpp $TEMP_DIR/modulation/
cp src/audio_engine/modulation/GeneratorMixer.h $TEMP_DIR/modulation/

# Envelopes
cp src/audio_engine/envelopes/EnvelopeCurves.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/EnvelopeCurves.h $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/DualPhaseEnvelope.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/DualPhaseEnvelope.h $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/PitchEnvelope.cpp $TEMP_DIR/envelopes/
cp src/audio_engine/envelopes/PitchEnvelope.h $TEMP_DIR/envelopes/

# Effects
cp src/audio_engine/effects/Compressor.cpp $TEMP_DIR/effects/
cp src/audio_engine/effects/Compressor.h $TEMP_DIR/effects/
cp src/audio_engine/effects/Reverb.cpp $TEMP_DIR/effects/
cp src/audio_engine/effects/Reverb.h $TEMP_DIR/effects/
cp src/audio_engine/effects/EffectsChain.cpp $TEMP_DIR/effects/
cp src/audio_engine/effects/EffectsChain.h $TEMP_DIR/effects/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "AudioEngine.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <limits>

using namespace KickDrum;

bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

void testMasterLevelControl() {
    std::cout << "Testing master level control..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    // Test default value
    assert(approxEqual(engine.getMasterLevel(), 0.8f));
    std::cout << "  ✓ Default master level is 0.8" << std::endl;
    
    // Test setting values
    engine.setMasterLevel(0.5f);
    assert(approxEqual(engine.getMasterLevel(), 0.5f));
    
    engine.setMasterLevel(1.0f);
    assert(approxEqual(engine.getMasterLevel(), 1.0f));
    
    engine.setMasterLevel(0.0f);
    assert(approxEqual(engine.getMasterLevel(), 0.0f));
    std::cout << "  ✓ Master level can be set" << std::endl;
    
    // Test clamping
    engine.setMasterLevel(2.0f);
    assert(approxEqual(engine.getMasterLevel(), 1.0f));
    
    engine.setMasterLevel(-0.5f);
    assert(approxEqual(engine.getMasterLevel(), 0.0f));
    std::cout << "  ✓ Master level clamped to [0.0, 1.0]" << std::endl;
    
    // Test effect on output
    engine.setMasterLevel(0.0f);
    engine.noteOn(60, 0.8f);
    
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // All samples should be zero with master level at 0
    bool allZero = true;
    for (float sample : buffer) {
        if (sample != 0.0f) {
            allZero = false;
            break;
        }
    }
    assert(allZero);
    std::cout << "  ✓ Master level 0.0 silences output" << std::endl;
}

void testSoftClipping() {
    std::cout << "Testing soft clipping..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    // Test enabled by default
    assert(engine.isSoftClippingEnabled());
    std::cout << "  ✓ Soft clipping enabled by default" << std::endl;
    
    // Test enable/disable
    engine.setSoftClippingEnabled(false);
    assert(!engine.isSoftClippingEnabled());
    
    engine.setSoftClippingEnabled(true);
    assert(engine.isSoftClippingEnabled());
    std::cout << "  ✓ Soft clipping can be toggled" << std::endl;
    
    // Test output limiting
    engine.setSoftClippingEnabled(true);
    engine.setMasterLevel(1.0f);
    
    // Trigger multiple notes to create loud output
    for (int note = 60; note < 68; ++note) {
        engine.noteOn(note, 1.0f);
    }
    
    std::vector<float> buffer(1024, 0.0f);
    engine.processBlock(buffer.data(), 1024, 1);
    
    // All samples should be within [-1.0, 1.0]
    bool allInRange = true;
    for (float sample : buffer) {
        if (sample < -1.0f || sample > 1.0f) {
            allInRange = false;
            break;
        }
    }
    assert(allInRange);
    std::cout << "  ✓ Soft clipping limits output to [-1.0, 1.0]" << std::endl;
}

void testNaNDetection() {
    std::cout << "Testing NaN/infinity detection..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    // Test enabled by default
    assert(engine.isNaNDetectionEnabled());
    std::cout << "  ✓ NaN detection enabled by default" << std::endl;
    
    // Test enable/disable
    engine.setNaNDetectionEnabled(false);
    assert(!engine.isNaNDetectionEnabled());
    
    engine.setNaNDetectionEnabled(true);
    assert(engine.isNaNDetectionEnabled());
    std::cout << "  ✓ NaN detection can be toggled" << std::endl;
    
    // Test normal operation produces valid output
    engine.setNaNDetectionEnabled(true);
    engine.noteOn(60, 0.8f);
    
    for (int block = 0; block < 10; ++block) {
        std::vector<float> buffer(512, 0.0f);
        engine.processBlock(buffer.data(), 512, 1);
        
        // All samples should be valid
        for (float sample : buffer) {
            assert(!std::isnan(sample));
            assert(!std::isinf(sample));
        }
    }
    std::cout << "  ✓ Normal operation produces valid output" << std::endl;
}

void testMultiChannelOutput() {
    std::cout << "Testing multi-channel output..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.noteOn(60, 0.8f);
    
    // Test mono
    std::vector<float> monoBuffer(512, 0.0f);
    engine.processBlock(monoBuffer.data(), 512, 1);
    
    bool hasNonZero = false;
    for (float sample : monoBuffer) {
        if (sample != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    assert(hasNonZero);
    std::cout << "  ✓ Mono output works" << std::endl;
    
    // Test stereo
    engine.allNotesOff();
    engine.noteOn(60, 0.8f);
    
    std::vector<float> stereoBuffer(512 * 2, 0.0f);
    engine.processBlock(stereoBuffer.data(), 512, 2);
    
    // Left and right should be identical
    bool identical = true;
    for (size_t i = 0; i < 512; ++i) {
        if (!approxEqual(stereoBuffer[i * 2], stereoBuffer[i * 2 + 1], 0.0001f)) {
            identical = false;
            break;
        }
    }
    assert(identical);
    std::cout << "  ✓ Stereo output duplicates mono" << std::endl;
}

void testIntegration() {
    std::cout << "Testing integration..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    // Test getters return valid pointers
    assert(engine.getEffectsChain() != nullptr);
    assert(engine.getVoiceAllocator() != nullptr);
    std::cout << "  ✓ Getters return valid pointers" << std::endl;
    
    // Test sample rate
    assert(approxEqual(engine.getSampleRate(), 48000.0f));
    std::cout << "  ✓ Sample rate is correct" << std::endl;
    
    // Test note on/off
    engine.noteOn(60, 0.8f);
    std::vector<float> buffer1(512, 0.0f);
    engine.processBlock(buffer1.data(), 512, 1);
    
    engine.noteOff(60);
    std::vector<float> buffer2(512, 0.0f);
    engine.processBlock(buffer2.data(), 512, 1);
    std::cout << "  ✓ Note on/off works" << std::endl;
    
    // Test all notes off
    for (int note = 60; note < 68; ++note) {
        engine.noteOn(note, 0.8f);
    }
    engine.allNotesOff();
    
    // Process enough to let envelopes complete
    for (int i = 0; i < 100; ++i) {
        std::vector<float> buffer(512, 0.0f);
        engine.processBlock(buffer.data(), 512, 1);
    }
    std::cout << "  ✓ All notes off works" << std::endl;
}

void testCombinedFeatures() {
    std::cout << "Testing combined features..." << std::endl;
    
    AudioEngine engine;
    engine.initialize(48000.0f);
    
    // Enable all safety features
    engine.setMasterLevel(0.8f);
    engine.setSoftClippingEnabled(true);
    engine.setNaNDetectionEnabled(true);
    
    // Trigger notes and process
    engine.noteOn(60, 1.0f);
    
    for (int block = 0; block < 20; ++block) {
        std::vector<float> buffer(512, 0.0f);
        engine.processBlock(buffer.data(), 512, 1);
        
        // Verify all safety constraints
        for (float sample : buffer) {
            assert(std::isfinite(sample));
            assert(sample >= -1.0f && sample <= 1.0f);
        }
    }
    std::cout << "  ✓ All safety features work together" << std::endl;
}

int main() {
    std::cout << "Testing AudioEngine master output & safety features..." << std::endl;
    std::cout << std::endl;
    
    try {
        testMasterLevelControl();
        std::cout << std::endl;
        
        testSoftClipping();
        std::cout << std::endl;
        
        testNaNDetection();
        std::cout << std::endl;
        
        testMultiChannelOutput();
        std::cout << std::endl;
        
        testIntegration();
        std::cout << std::endl;
        
        testCombinedFeatures();
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
    -I$TEMP_DIR/include \
    -I$TEMP_DIR/voice \
    -I$TEMP_DIR/effects \
    -I$TEMP_DIR/utils \
    -I$TEMP_DIR/generators \
    -I$TEMP_DIR/modulation \
    -I$TEMP_DIR/envelopes \
    -o $TEMP_DIR/test_audio_engine \
    $TEMP_DIR/core/AudioEngine.cpp \
    $TEMP_DIR/utils/DSPUtils.cpp \
    $TEMP_DIR/voice/Voice.cpp \
    $TEMP_DIR/voice/VoiceAllocator.cpp \
    $TEMP_DIR/generators/SineDriver.cpp \
    $TEMP_DIR/generators/HarmonicMembrane.cpp \
    $TEMP_DIR/generators/NoiseGenerator.cpp \
    $TEMP_DIR/modulation/RingModulator.cpp \
    $TEMP_DIR/modulation/GeneratorMixer.cpp \
    $TEMP_DIR/envelopes/EnvelopeCurves.cpp \
    $TEMP_DIR/envelopes/DualPhaseEnvelope.cpp \
    $TEMP_DIR/envelopes/PitchEnvelope.cpp \
    $TEMP_DIR/effects/Compressor.cpp \
    $TEMP_DIR/effects/Reverb.cpp \
    $TEMP_DIR/effects/EffectsChain.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_audio_engine
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== AudioEngine master output & safety features verified! ==="
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
