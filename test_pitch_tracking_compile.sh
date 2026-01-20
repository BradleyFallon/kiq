#!/bin/bash

# Test script for pitch tracking functionality
# Tests Requirements 4.7 and 13.4

set -e

echo "=== Testing Pitch Tracking Compilation ==="

# Create a temporary directory for compilation
TEMP_DIR=$(mktemp -d)
echo "Using temp directory: $TEMP_DIR"

# Copy source files
mkdir -p $TEMP_DIR/generators
mkdir -p $TEMP_DIR/modulation
mkdir -p $TEMP_DIR/envelopes
mkdir -p $TEMP_DIR/voice
mkdir -p $TEMP_DIR/utils

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
cp src/audio_engine/voice/VoiceAllocator.cpp $TEMP_DIR/voice/
cp src/audio_engine/voice/VoiceAllocator.h $TEMP_DIR/voice/

# Copy utils
cp src/audio_engine/utils/DSPUtils.cpp $TEMP_DIR/utils/
cp src/audio_engine/utils/DSPUtils.h $TEMP_DIR/utils/

# Create a test file
cat > $TEMP_DIR/test_pitch_tracking.cpp << 'EOF'
#include "voice/Voice.h"
#include "voice/VoiceAllocator.h"
#include "utils/DSPUtils.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace KickDrum;

bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

void testMIDINoteToFrequency() {
    std::cout << "Testing MIDI note to frequency conversion..." << std::endl;
    
    // Test standard MIDI tuning reference: A4 (note 69) = 440 Hz
    float freq69 = DSPUtils::midiNoteToFrequency(69);
    assert(approxEqual(freq69, 440.0f));
    std::cout << "  ✓ MIDI note 69 (A4) = " << freq69 << " Hz (expected 440 Hz)" << std::endl;
    
    // Test C4 (middle C, note 60) = 261.63 Hz
    float freq60 = DSPUtils::midiNoteToFrequency(60);
    assert(approxEqual(freq60, 261.63f));
    std::cout << "  ✓ MIDI note 60 (C4) = " << freq60 << " Hz (expected 261.63 Hz)" << std::endl;
    
    // Test C2 (note 36) = 65.41 Hz (typical kick drum range)
    float freq36 = DSPUtils::midiNoteToFrequency(36);
    assert(approxEqual(freq36, 65.41f));
    std::cout << "  ✓ MIDI note 36 (C2) = " << freq36 << " Hz (expected 65.41 Hz)" << std::endl;
    
    // Test C1 (note 24) = 32.70 Hz (low kick drum)
    float freq24 = DSPUtils::midiNoteToFrequency(24);
    assert(approxEqual(freq24, 32.70f));
    std::cout << "  ✓ MIDI note 24 (C1) = " << freq24 << " Hz (expected 32.70 Hz)" << std::endl;
    
    // Test octave relationship
    float freq48 = DSPUtils::midiNoteToFrequency(48);
    assert(approxEqual(freq60 / freq48, 2.0f));
    std::cout << "  ✓ Octave relationship verified (C4/C3 = 2.0)" << std::endl;
}

void testPitchTrackingEnableDisable() {
    std::cout << "\nTesting pitch tracking enable/disable..." << std::endl;
    
    Voice voice;
    voice.initialize(48000.0f);
    
    // Pitch tracking should be enabled by default
    assert(voice.isPitchTrackingEnabled());
    std::cout << "  ✓ Pitch tracking enabled by default" << std::endl;
    
    // Disable pitch tracking
    voice.setPitchTrackingEnabled(false);
    assert(!voice.isPitchTrackingEnabled());
    std::cout << "  ✓ Pitch tracking can be disabled" << std::endl;
    
    // Re-enable pitch tracking
    voice.setPitchTrackingEnabled(true);
    assert(voice.isPitchTrackingEnabled());
    std::cout << "  ✓ Pitch tracking can be re-enabled" << std::endl;
}

void testPitchTrackingAffectsBasePitch() {
    std::cout << "\nTesting pitch tracking affects base pitch..." << std::endl;
    
    Voice voice;
    voice.initialize(48000.0f);
    voice.setPitchTrackingEnabled(true);
    
    // Trigger with MIDI note 36 (C2 = 65.41 Hz)
    voice.trigger(36, 1.0f);
    assert(approxEqual(voice.getBasePitch(), 65.41f));
    std::cout << "  ✓ Note 36 sets pitch to " << voice.getBasePitch() << " Hz (expected 65.41 Hz)" << std::endl;
    
    // Trigger with MIDI note 48 (C3 = 130.81 Hz)
    voice.trigger(48, 1.0f);
    assert(approxEqual(voice.getBasePitch(), 130.81f));
    std::cout << "  ✓ Note 48 sets pitch to " << voice.getBasePitch() << " Hz (expected 130.81 Hz)" << std::endl;
    
    // Trigger with MIDI note 60 (C4 = 261.63 Hz)
    voice.trigger(60, 1.0f);
    assert(approxEqual(voice.getBasePitch(), 261.63f));
    std::cout << "  ✓ Note 60 sets pitch to " << voice.getBasePitch() << " Hz (expected 261.63 Hz)" << std::endl;
}

void testPitchTrackingDisabled() {
    std::cout << "\nTesting pitch tracking disabled..." << std::endl;
    
    Voice voice;
    voice.initialize(48000.0f);
    
    // Set a specific base pitch
    float originalPitch = 50.0f;
    voice.setBasePitch(originalPitch);
    
    // Disable pitch tracking
    voice.setPitchTrackingEnabled(false);
    
    // Trigger with different MIDI notes
    voice.trigger(36, 1.0f);
    assert(approxEqual(voice.getBasePitch(), originalPitch));
    std::cout << "  ✓ Note 36 does not change pitch (remains " << voice.getBasePitch() << " Hz)" << std::endl;
    
    voice.trigger(48, 1.0f);
    assert(approxEqual(voice.getBasePitch(), originalPitch));
    std::cout << "  ✓ Note 48 does not change pitch (remains " << voice.getBasePitch() << " Hz)" << std::endl;
    
    voice.trigger(60, 1.0f);
    assert(approxEqual(voice.getBasePitch(), originalPitch));
    std::cout << "  ✓ Note 60 does not change pitch (remains " << voice.getBasePitch() << " Hz)" << std::endl;
}

void testSetPitchFromMIDINote() {
    std::cout << "\nTesting setPitchFromMIDINote..." << std::endl;
    
    Voice voice;
    voice.initialize(48000.0f);
    
    voice.setPitchFromMIDINote(36);
    assert(approxEqual(voice.getBasePitch(), 65.41f));
    std::cout << "  ✓ setPitchFromMIDINote(36) = " << voice.getBasePitch() << " Hz" << std::endl;
    
    voice.setPitchFromMIDINote(48);
    assert(approxEqual(voice.getBasePitch(), 130.81f));
    std::cout << "  ✓ setPitchFromMIDINote(48) = " << voice.getBasePitch() << " Hz" << std::endl;
    
    voice.setPitchFromMIDINote(60);
    assert(approxEqual(voice.getBasePitch(), 261.63f));
    std::cout << "  ✓ setPitchFromMIDINote(60) = " << voice.getBasePitch() << " Hz" << std::endl;
}

void testVoiceAllocatorPitchTracking() {
    std::cout << "\nTesting VoiceAllocator pitch tracking..." << std::endl;
    
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    
    // Enable pitch tracking for all voices
    allocator.setPitchTrackingEnabled(true);
    
    // Verify all voices have pitch tracking enabled
    for (int i = 0; i < allocator.getNumVoices(); ++i) {
        assert(allocator.getVoice(i).isPitchTrackingEnabled());
    }
    std::cout << "  ✓ All voices have pitch tracking enabled" << std::endl;
    
    // Disable pitch tracking for all voices
    allocator.setPitchTrackingEnabled(false);
    
    // Verify all voices have pitch tracking disabled
    for (int i = 0; i < allocator.getNumVoices(); ++i) {
        assert(!allocator.getVoice(i).isPitchTrackingEnabled());
    }
    std::cout << "  ✓ All voices have pitch tracking disabled" << std::endl;
}

void testAllocatedVoicesUseMIDIPitch() {
    std::cout << "\nTesting allocated voices use MIDI pitch..." << std::endl;
    
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    allocator.setPitchTrackingEnabled(true);
    
    // Allocate voice with MIDI note 36
    Voice* voice1 = allocator.allocateVoice(36, 1.0f);
    assert(voice1 != nullptr);
    assert(approxEqual(voice1->getBasePitch(), 65.41f));
    std::cout << "  ✓ Voice with note 36 has pitch " << voice1->getBasePitch() << " Hz" << std::endl;
    
    // Allocate voice with MIDI note 48
    Voice* voice2 = allocator.allocateVoice(48, 1.0f);
    assert(voice2 != nullptr);
    assert(approxEqual(voice2->getBasePitch(), 130.81f));
    std::cout << "  ✓ Voice with note 48 has pitch " << voice2->getBasePitch() << " Hz" << std::endl;
    
    // Allocate voice with MIDI note 60
    Voice* voice3 = allocator.allocateVoice(60, 1.0f);
    assert(voice3 != nullptr);
    assert(approxEqual(voice3->getBasePitch(), 261.63f));
    std::cout << "  ✓ Voice with note 60 has pitch " << voice3->getBasePitch() << " Hz" << std::endl;
}

void testKickDrumPitchRange() {
    std::cout << "\nTesting kick drum pitch range..." << std::endl;
    
    Voice voice;
    voice.initialize(48000.0f);
    voice.setPitchTrackingEnabled(true);
    
    // C1 (note 24) = 32.70 Hz - very low kick
    voice.trigger(24, 1.0f);
    assert(approxEqual(voice.getBasePitch(), 32.70f));
    std::cout << "  ✓ C1 (note 24) = " << voice.getBasePitch() << " Hz (very low kick)" << std::endl;
    
    // C2 (note 36) = 65.41 Hz - typical kick drum
    voice.trigger(36, 1.0f);
    assert(approxEqual(voice.getBasePitch(), 65.41f));
    std::cout << "  ✓ C2 (note 36) = " << voice.getBasePitch() << " Hz (typical kick)" << std::endl;
    
    // C3 (note 48) = 130.81 Hz - high kick
    voice.trigger(48, 1.0f);
    assert(approxEqual(voice.getBasePitch(), 130.81f));
    std::cout << "  ✓ C3 (note 48) = " << voice.getBasePitch() << " Hz (high kick)" << std::endl;
}

int main() {
    std::cout << "=== Testing Pitch Tracking Implementation ===" << std::endl;
    std::cout << std::endl;
    
    try {
        testMIDINoteToFrequency();
        testPitchTrackingEnableDisable();
        testPitchTrackingAffectsBasePitch();
        testPitchTrackingDisabled();
        testSetPitchFromMIDINote();
        testVoiceAllocatorPitchTracking();
        testAllocatedVoicesUseMIDIPitch();
        testKickDrumPitchRange();
        
        std::cout << std::endl;
        std::cout << "=== All Pitch Tracking Tests Passed! ===" << std::endl;
        std::cout << std::endl;
        std::cout << "Requirements validated:" << std::endl;
        std::cout << "  ✓ 4.7: Pitch tracking parameter allowing MIDI note to affect base pitch" << std::endl;
        std::cout << "  ✓ 13.4: Different MIDI note numbers adjust base pitch accordingly" << std::endl;
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
    -I$TEMP_DIR/utils \
    -o $TEMP_DIR/test_pitch_tracking \
    $TEMP_DIR/generators/SineDriver.cpp \
    $TEMP_DIR/generators/HarmonicMembrane.cpp \
    $TEMP_DIR/generators/NoiseGenerator.cpp \
    $TEMP_DIR/modulation/RingModulator.cpp \
    $TEMP_DIR/envelopes/EnvelopeCurves.cpp \
    $TEMP_DIR/envelopes/DualPhaseEnvelope.cpp \
    $TEMP_DIR/envelopes/PitchEnvelope.cpp \
    $TEMP_DIR/voice/Voice.cpp \
    $TEMP_DIR/voice/VoiceAllocator.cpp \
    $TEMP_DIR/utils/DSPUtils.cpp \
    $TEMP_DIR/test_pitch_tracking.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo ""
    echo "Running tests..."
    echo ""
    $TEMP_DIR/test_pitch_tracking
    TEST_RESULT=$?
    echo ""
    
    # Cleanup
    rm -rf $TEMP_DIR
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo "=== Pitch Tracking implementation verified! ==="
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

