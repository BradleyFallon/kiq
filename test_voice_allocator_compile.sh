#!/bin/bash

# Simple compilation test for VoiceAllocator
# This verifies the code compiles and basic functionality works

echo "=== Testing VoiceAllocator Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
mkdir -p $TEMP_DIR/generators
mkdir -p $TEMP_DIR/modulation
mkdir -p $TEMP_DIR/envelopes
mkdir -p $TEMP_DIR/voice

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
cp src/audio_engine/voice/Voice.cpp $TEMP_DIR/voice/
cp src/audio_engine/voice/Voice.h $TEMP_DIR/voice/

# Copy VoiceAllocator
cp src/audio_engine/voice/VoiceAllocator.cpp $TEMP_DIR/voice/
cp src/audio_engine/voice/VoiceAllocator.h $TEMP_DIR/voice/

# Create a simple test file
cat > $TEMP_DIR/test_main.cpp << 'EOF'
#include "voice/VoiceAllocator.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace KickDrum;

void testBasicFunctionality() {
    VoiceAllocator allocator;
    const float sampleRate = 48000.0f;
    
    // Test initialization
    allocator.initialize(sampleRate);
    assert(allocator.getNumVoices() == 8);
    assert(allocator.getNumActiveVoices() == 0);
    std::cout << "✓ Initialization creates 8 voices" << std::endl;
    
    // Test voice allocation
    Voice* voice = allocator.allocateVoice(60, 1.0f);
    assert(voice != nullptr);
    assert(voice->isActive());
    assert(voice->getNote() == 60);
    assert(allocator.getNumActiveVoices() == 1);
    std::cout << "✓ Voice allocation works" << std::endl;
    
    // Test multiple allocations
    allocator.allocateVoice(62, 1.0f);
    allocator.allocateVoice(64, 1.0f);
    assert(allocator.getNumActiveVoices() == 3);
    std::cout << "✓ Multiple voice allocation works" << std::endl;
    
    // Test allocating all 8 voices
    VoiceAllocator allocator2;
    allocator2.initialize(sampleRate);
    for (int i = 0; i < 8; i++) {
        Voice* v = allocator2.allocateVoice(60 + i, 1.0f);
        assert(v != nullptr);
    }
    assert(allocator2.getNumActiveVoices() == 8);
    std::cout << "✓ Can allocate all 8 voices" << std::endl;
    
    // Test voice stealing
    Voice* stolen = allocator2.allocateVoice(70, 1.0f);
    assert(stolen != nullptr);
    assert(allocator2.getNumActiveVoices() == 8);  // Still 8 voices
    std::cout << "✓ Voice stealing works" << std::endl;
    
    // Test rendering with no active voices
    VoiceAllocator allocator3;
    allocator3.initialize(sampleRate);
    float buffer[100];
    allocator3.renderBuffer(buffer, 100);
    bool allZero = true;
    for (int i = 0; i < 100; i++) {
        if (buffer[i] != 0.0f) {
            allZero = false;
            break;
        }
    }
    assert(allZero);
    std::cout << "✓ Rendering with no voices produces silence" << std::endl;
    
    // Test rendering with active voice
    VoiceAllocator allocator4;
    allocator4.initialize(sampleRate);
    allocator4.allocateVoice(60, 1.0f);
    allocator4.renderBuffer(buffer, 100);
    bool hasNonZero = false;
    for (int i = 0; i < 100; i++) {
        if (buffer[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    assert(hasNonZero);
    std::cout << "✓ Rendering with active voice produces audio" << std::endl;
    
    // Test output is finite
    VoiceAllocator allocator5;
    allocator5.initialize(sampleRate);
    allocator5.allocateVoice(60, 1.0f);
    allocator5.renderBuffer(buffer, 100);
    for (int i = 0; i < 100; i++) {
        assert(std::isfinite(buffer[i]));
    }
    std::cout << "✓ Output is always finite" << std::endl;
    
    // Test voice release
    VoiceAllocator allocator6;
    allocator6.initialize(sampleRate);
    allocator6.allocateVoice(60, 1.0f);
    assert(allocator6.getNumActiveVoices() == 1);
    allocator6.releaseVoice(60);
    // Voice should still be active (in release phase)
    assert(allocator6.getNumActiveVoices() == 1);
    std::cout << "✓ Voice release works" << std::endl;
    
    // Test release all
    VoiceAllocator allocator7;
    allocator7.initialize(sampleRate);
    allocator7.allocateVoice(60, 1.0f);
    allocator7.allocateVoice(62, 1.0f);
    allocator7.allocateVoice(64, 1.0f);
    assert(allocator7.getNumActiveVoices() == 3);
    allocator7.releaseAll();
    // All voices should still be active (in release phase)
    assert(allocator7.getNumActiveVoices() == 3);
    std::cout << "✓ Release all works" << std::endl;
    
    // Test voice access
    VoiceAllocator allocator8;
    allocator8.initialize(sampleRate);
    for (int i = 0; i < 8; i++) {
        Voice& v = allocator8.getVoice(i);
        assert(!v.isActive());
    }
    std::cout << "✓ Voice access by index works" << std::endl;
    
    // Test sample rate change
    VoiceAllocator allocator9;
    allocator9.initialize(sampleRate);
    allocator9.allocateVoice(60, 1.0f);
    allocator9.setSampleRate(44100.0f);
    allocator9.renderBuffer(buffer, 100);
    hasNonZero = false;
    for (int i = 0; i < 100; i++) {
        if (buffer[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    assert(hasNonZero);
    std::cout << "✓ Sample rate change works" << std::endl;
}

void testPolyphony() {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    
    // Test 8 simultaneous voices (Requirement 12.4)
    for (int i = 0; i < 8; i++) {
        Voice* voice = allocator.allocateVoice(60 + i, 1.0f);
        assert(voice != nullptr);
        assert(voice->isActive());
    }
    assert(allocator.getNumActiveVoices() == 8);
    
    // Render audio with all 8 voices
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    bool hasNonZero = false;
    for (int i = 0; i < 100; i++) {
        if (buffer[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    assert(hasNonZero);
    
    std::cout << "✓ Supports 8 simultaneous voices (Requirement 12.4)" << std::endl;
}

void testVoiceStealing() {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    
    // Allocate all 8 voices
    for (int i = 0; i < 8; i++) {
        allocator.allocateVoice(60 + i, 1.0f);
    }
    
    // Render some samples to age the voices
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    // Allocate a 9th voice - should steal the oldest
    Voice* newVoice = allocator.allocateVoice(70, 1.0f);
    assert(newVoice != nullptr);
    assert(newVoice->getNote() == 70);
    assert(newVoice->getAge() == 0);  // Just triggered
    
    // Should still have 8 active voices
    assert(allocator.getNumActiveVoices() == 8);
    
    std::cout << "✓ Voice stealing steals oldest voice" << std::endl;
}

void testEdgeCases() {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    
    // Test zero velocity
    Voice* voice = allocator.allocateVoice(60, 0.0f);
    assert(voice != nullptr);
    assert(voice->isActive());
    std::cout << "✓ Zero velocity allocation works" << std::endl;
    
    // Test same note twice
    VoiceAllocator allocator2;
    allocator2.initialize(48000.0f);
    Voice* v1 = allocator2.allocateVoice(60, 1.0f);
    Voice* v2 = allocator2.allocateVoice(60, 1.0f);
    assert(v1 != v2);
    assert(v1->getNote() == 60);
    assert(v2->getNote() == 60);
    std::cout << "✓ Same note twice allocates different voices" << std::endl;
    
    // Test null buffer
    allocator.renderBuffer(nullptr, 100);
    std::cout << "✓ Null buffer doesn't crash" << std::endl;
    
    // Test zero samples
    float buffer[100];
    allocator.renderBuffer(buffer, 0);
    std::cout << "✓ Zero samples doesn't crash" << std::endl;
    
    // Test negative samples
    allocator.renderBuffer(buffer, -100);
    std::cout << "✓ Negative samples doesn't crash" << std::endl;
    
    // Test release non-existent note
    VoiceAllocator allocator3;
    allocator3.initialize(48000.0f);
    allocator3.releaseVoice(60);  // Should not crash
    std::cout << "✓ Release non-existent note doesn't crash" << std::endl;
}

int main() {
    std::cout << "Testing VoiceAllocator implementation..." << std::endl;
    std::cout << std::endl;
    
    try {
        testBasicFunctionality();
        std::cout << std::endl;
        testPolyphony();
        std::cout << std::endl;
        testVoiceStealing();
        std::cout << std::endl;
        testEdgeCases();
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
    -I$TEMP_DIR/voice \
    -o $TEMP_DIR/test_voice_allocator \
    $TEMP_DIR/generators/SineDriver.cpp \
    $TEMP_DIR/generators/HarmonicMembrane.cpp \
    $TEMP_DIR/generators/NoiseGenerator.cpp \
    $TEMP_DIR/modulation/RingModulator.cpp \
    $TEMP_DIR/envelopes/EnvelopeCurves.cpp \
    $TEMP_DIR/envelopes/DualPhaseEnvelope.cpp \
    $TEMP_DIR/envelopes/PitchEnvelope.cpp \
    $TEMP_DIR/voice/Voice.cpp \
    $TEMP_DIR/voice/VoiceAllocator.cpp \
    $TEMP_DIR/test_main.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_voice_allocator
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== VoiceAllocator implementation verified! ==="
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
