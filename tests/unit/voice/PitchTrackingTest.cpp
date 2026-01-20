#include <gtest/gtest.h>
#include "../../../src/audio_engine/voice/Voice.h"
#include "../../../src/audio_engine/voice/VoiceAllocator.h"
#include "../../../src/audio_engine/utils/DSPUtils.h"
#include <cmath>

namespace KickDrum {

/**
 * @brief Test fixture for pitch tracking functionality
 * 
 * Tests Requirements 4.7 and 13.4:
 * - 4.7: Pitch tracking parameter allowing MIDI note number to affect base pitch
 * - 13.4: Different MIDI note numbers adjust base pitch accordingly
 */
class PitchTrackingTest : public ::testing::Test {
protected:
    void SetUp() override {
        voice.initialize(48000.0f);
    }
    
    Voice voice;
};

/**
 * @brief Test MIDI note to frequency conversion utility
 * 
 * Validates that the midiNoteToFrequency function correctly converts
 * MIDI note numbers to frequencies using the standard MIDI tuning formula.
 */
TEST_F(PitchTrackingTest, MIDINoteToFrequencyConversion) {
    // Test standard MIDI tuning reference: A4 (note 69) = 440 Hz
    float freq69 = DSPUtils::midiNoteToFrequency(69);
    EXPECT_NEAR(freq69, 440.0f, 0.01f);
    
    // Test C4 (middle C, note 60) = 261.63 Hz
    float freq60 = DSPUtils::midiNoteToFrequency(60);
    EXPECT_NEAR(freq60, 261.63f, 0.01f);
    
    // Test C2 (note 36) = 65.41 Hz (typical kick drum range)
    float freq36 = DSPUtils::midiNoteToFrequency(36);
    EXPECT_NEAR(freq36, 65.41f, 0.01f);
    
    // Test C1 (note 24) = 32.70 Hz (low kick drum)
    float freq24 = DSPUtils::midiNoteToFrequency(24);
    EXPECT_NEAR(freq24, 32.70f, 0.01f);
    
    // Test octave relationship: each octave doubles frequency
    float freq48 = DSPUtils::midiNoteToFrequency(48);  // C3
    EXPECT_NEAR(freq60 / freq48, 2.0f, 0.01f);  // C4 is one octave above C3
}

/**
 * @brief Test pitch tracking enable/disable
 * 
 * Validates that pitch tracking can be enabled and disabled,
 * and that the state is correctly reported.
 */
TEST_F(PitchTrackingTest, PitchTrackingEnableDisable) {
    // Pitch tracking should be enabled by default
    EXPECT_TRUE(voice.isPitchTrackingEnabled());
    
    // Disable pitch tracking
    voice.setPitchTrackingEnabled(false);
    EXPECT_FALSE(voice.isPitchTrackingEnabled());
    
    // Re-enable pitch tracking
    voice.setPitchTrackingEnabled(true);
    EXPECT_TRUE(voice.isPitchTrackingEnabled());
}

/**
 * @brief Test pitch tracking affects base pitch when enabled
 * 
 * Validates that when pitch tracking is enabled, triggering a voice
 * with different MIDI notes results in different base pitches.
 */
TEST_F(PitchTrackingTest, PitchTrackingAffectsBasePitch) {
    // Enable pitch tracking
    voice.setPitchTrackingEnabled(true);
    
    // Trigger with MIDI note 36 (C2 = 65.41 Hz)
    voice.trigger(36, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), 65.41f, 0.01f);
    
    // Trigger with MIDI note 48 (C3 = 130.81 Hz)
    voice.trigger(48, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), 130.81f, 0.01f);
    
    // Trigger with MIDI note 60 (C4 = 261.63 Hz)
    voice.trigger(60, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), 261.63f, 0.01f);
}

/**
 * @brief Test pitch tracking does not affect base pitch when disabled
 * 
 * Validates that when pitch tracking is disabled, triggering a voice
 * with different MIDI notes does NOT change the base pitch.
 */
TEST_F(PitchTrackingTest, PitchTrackingDisabledDoesNotAffectBasePitch) {
    // Set a specific base pitch
    float originalPitch = 50.0f;
    voice.setBasePitch(originalPitch);
    
    // Disable pitch tracking
    voice.setPitchTrackingEnabled(false);
    
    // Trigger with different MIDI notes
    voice.trigger(36, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), originalPitch, 0.01f);
    
    voice.trigger(48, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), originalPitch, 0.01f);
    
    voice.trigger(60, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), originalPitch, 0.01f);
}

/**
 * @brief Test setPitchFromMIDINote method
 * 
 * Validates that the setPitchFromMIDINote method correctly converts
 * MIDI note numbers to frequencies and sets the base pitch.
 */
TEST_F(PitchTrackingTest, SetPitchFromMIDINote) {
    // Test various MIDI notes
    voice.setPitchFromMIDINote(36);
    EXPECT_NEAR(voice.getBasePitch(), 65.41f, 0.01f);
    
    voice.setPitchFromMIDINote(48);
    EXPECT_NEAR(voice.getBasePitch(), 130.81f, 0.01f);
    
    voice.setPitchFromMIDINote(60);
    EXPECT_NEAR(voice.getBasePitch(), 261.63f, 0.01f);
    
    voice.setPitchFromMIDINote(69);
    EXPECT_NEAR(voice.getBasePitch(), 440.0f, 0.01f);
}

/**
 * @brief Test pitch tracking with VoiceAllocator
 * 
 * Validates that the VoiceAllocator correctly propagates pitch tracking
 * state to all voices in the pool.
 */
TEST(VoiceAllocatorPitchTrackingTest, SetPitchTrackingForAllVoices) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    
    // Enable pitch tracking for all voices
    allocator.setPitchTrackingEnabled(true);
    
    // Verify all voices have pitch tracking enabled
    for (int i = 0; i < allocator.getNumVoices(); ++i) {
        EXPECT_TRUE(allocator.getVoice(i).isPitchTrackingEnabled());
    }
    
    // Disable pitch tracking for all voices
    allocator.setPitchTrackingEnabled(false);
    
    // Verify all voices have pitch tracking disabled
    for (int i = 0; i < allocator.getNumVoices(); ++i) {
        EXPECT_FALSE(allocator.getVoice(i).isPitchTrackingEnabled());
    }
}

/**
 * @brief Test pitch tracking with voice allocation
 * 
 * Validates that when pitch tracking is enabled, allocated voices
 * have their pitch set according to the MIDI note number.
 */
TEST(VoiceAllocatorPitchTrackingTest, AllocatedVoicesUseMIDIPitch) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    
    // Enable pitch tracking
    allocator.setPitchTrackingEnabled(true);
    
    // Allocate voice with MIDI note 36
    Voice* voice1 = allocator.allocateVoice(36, 1.0f);
    ASSERT_NE(voice1, nullptr);
    EXPECT_NEAR(voice1->getBasePitch(), 65.41f, 0.01f);
    
    // Allocate voice with MIDI note 48
    Voice* voice2 = allocator.allocateVoice(48, 1.0f);
    ASSERT_NE(voice2, nullptr);
    EXPECT_NEAR(voice2->getBasePitch(), 130.81f, 0.01f);
    
    // Allocate voice with MIDI note 60
    Voice* voice3 = allocator.allocateVoice(60, 1.0f);
    ASSERT_NE(voice3, nullptr);
    EXPECT_NEAR(voice3->getBasePitch(), 261.63f, 0.01f);
}

/**
 * @brief Test pitch tracking disabled with voice allocation
 * 
 * Validates that when pitch tracking is disabled, allocated voices
 * maintain their base pitch regardless of MIDI note number.
 */
TEST(VoiceAllocatorPitchTrackingTest, AllocatedVoicesIgnoreMIDIPitchWhenDisabled) {
    VoiceAllocator allocator;
    allocator.initialize(48000.0f);
    
    // Set a specific base pitch for all voices
    float basePitch = 50.0f;
    for (int i = 0; i < allocator.getNumVoices(); ++i) {
        allocator.getVoice(i).setBasePitch(basePitch);
    }
    
    // Disable pitch tracking
    allocator.setPitchTrackingEnabled(false);
    
    // Allocate voices with different MIDI notes
    Voice* voice1 = allocator.allocateVoice(36, 1.0f);
    ASSERT_NE(voice1, nullptr);
    EXPECT_NEAR(voice1->getBasePitch(), basePitch, 0.01f);
    
    Voice* voice2 = allocator.allocateVoice(48, 1.0f);
    ASSERT_NE(voice2, nullptr);
    EXPECT_NEAR(voice2->getBasePitch(), basePitch, 0.01f);
    
    Voice* voice3 = allocator.allocateVoice(60, 1.0f);
    ASSERT_NE(voice3, nullptr);
    EXPECT_NEAR(voice3->getBasePitch(), basePitch, 0.01f);
}

/**
 * @brief Test pitch range for kick drum synthesis
 * 
 * Validates that MIDI notes in the typical kick drum range (C1 to C3)
 * produce appropriate frequencies.
 */
TEST_F(PitchTrackingTest, KickDrumPitchRange) {
    voice.setPitchTrackingEnabled(true);
    
    // C1 (note 24) = 32.70 Hz - very low kick
    voice.trigger(24, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), 32.70f, 0.01f);
    
    // C2 (note 36) = 65.41 Hz - typical kick drum
    voice.trigger(36, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), 65.41f, 0.01f);
    
    // C3 (note 48) = 130.81 Hz - high kick
    voice.trigger(48, 1.0f);
    EXPECT_NEAR(voice.getBasePitch(), 130.81f, 0.01f);
}

} // namespace KickDrum
