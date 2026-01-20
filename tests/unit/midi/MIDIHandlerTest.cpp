#include <gtest/gtest.h>
#include "../../../src/audio_engine/midi/MIDIHandler.h"
#include "../../../src/audio_engine/midi/MIDIMessage.h"
#include "../../../src/audio_engine/voice/VoiceAllocator.h"
#include "../../../src/audio_engine/parameters/ParameterManager.h"

using namespace KickDrum;

/**
 * @brief Test fixture for MIDIHandler tests
 */
class MIDIHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize voice allocator with sample rate
        voiceAllocator.initialize(44100.0f);
        
        // Create parameter manager and register parameters
        parameterManager = std::make_unique<ParameterManager>();
        parameterManager->registerAllSynthesisParameters();
        
        // Create MIDI handler with voice allocator and parameter manager
        midiHandler = std::make_unique<MIDIHandler>(&voiceAllocator, parameterManager.get());
    }
    
    VoiceAllocator voiceAllocator;
    std::unique_ptr<ParameterManager> parameterManager;
    std::unique_ptr<MIDIHandler> midiHandler;
};

// ============================================================================
// Constructor Tests
// ============================================================================

TEST_F(MIDIHandlerTest, ConstructorSetsVoiceAllocator) {
    EXPECT_EQ(midiHandler->getVoiceAllocator(), &voiceAllocator);
}

TEST_F(MIDIHandlerTest, ConstructorSetsParameterManager) {
    EXPECT_EQ(midiHandler->getParameterManager(), parameterManager.get());
}

TEST_F(MIDIHandlerTest, ConstructorWithNullVoiceAllocator) {
    MIDIHandler handler(nullptr);
    EXPECT_EQ(handler.getVoiceAllocator(), nullptr);
}

TEST_F(MIDIHandlerTest, ConstructorWithNullParameterManager) {
    MIDIHandler handler(&voiceAllocator, nullptr);
    EXPECT_EQ(handler.getParameterManager(), nullptr);
}

// ============================================================================
// Note-On Handling Tests
// ============================================================================

TEST_F(MIDIHandlerTest, HandleNoteOnAllocatesVoice) {
    // Initially no voices should be active
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 0);
    
    // Send note-on message
    midiHandler->handleNoteOn(60, 100);
    
    // One voice should now be active
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandleNoteOnSetsCorrectNote) {
    // Send note-on for middle C (60)
    midiHandler->handleNoteOn(60, 100);
    
    // Find the active voice and check its note
    for (int i = 0; i < voiceAllocator.getNumVoices(); ++i) {
        const Voice& voice = voiceAllocator.getVoice(i);
        if (voice.isActive()) {
            EXPECT_EQ(voice.getNote(), 60);
            return;
        }
    }
    
    FAIL() << "No active voice found";
}

TEST_F(MIDIHandlerTest, HandleNoteOnNormalizesVelocity) {
    // Send note-on with max velocity (127)
    midiHandler->handleNoteOn(60, 127);
    
    // The velocity should be normalized to 1.0
    // We can't directly check the velocity, but we can verify the voice is active
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandleNoteOnWithZeroVelocity) {
    // Note-on with velocity 0 should still allocate a voice
    // (The MIDIMessage.isNoteOn() method treats velocity 0 as note-off,
    // but handleNoteOn() itself doesn't check this)
    midiHandler->handleNoteOn(60, 0);
    
    // Voice should be allocated (with velocity 0.0)
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandleNoteOnWithMinVelocity) {
    // Send note-on with minimum non-zero velocity
    midiHandler->handleNoteOn(60, 1);
    
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandleNoteOnWithMaxVelocity) {
    // Send note-on with maximum velocity
    midiHandler->handleNoteOn(60, 127);
    
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandleNoteOnWithNullVoiceAllocator) {
    // Create handler with null voice allocator
    MIDIHandler handler(nullptr);
    
    // Should not crash
    EXPECT_NO_THROW(handler.handleNoteOn(60, 100));
}

// ============================================================================
// Note-Off Handling Tests
// ============================================================================

TEST_F(MIDIHandlerTest, HandleNoteOffReleasesVoice) {
    // Trigger a note
    midiHandler->handleNoteOn(60, 100);
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
    
    // Release the note
    midiHandler->handleNoteOff(60);
    
    // Voice should still be active (in release phase)
    // but will eventually become inactive
    // For now, we just verify the call doesn't crash
    EXPECT_NO_THROW(midiHandler->handleNoteOff(60));
}

TEST_F(MIDIHandlerTest, HandleNoteOffForNonExistentNote) {
    // Release a note that was never triggered
    // Should not crash
    EXPECT_NO_THROW(midiHandler->handleNoteOff(60));
}

TEST_F(MIDIHandlerTest, HandleNoteOffWithNullVoiceAllocator) {
    // Create handler with null voice allocator
    MIDIHandler handler(nullptr);
    
    // Should not crash
    EXPECT_NO_THROW(handler.handleNoteOff(60));
}

// ============================================================================
// MIDI Message Processing Tests
// ============================================================================

TEST_F(MIDIHandlerTest, ProcessMIDIMessageNoteOn) {
    // Create a note-on message
    MIDIMessage message(MIDIMessageType::NOTE_ON, 0, 60, 100, 0);
    
    // Process the message
    midiHandler->processMIDIMessage(message);
    
    // Voice should be allocated
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, ProcessMIDIMessageNoteOff) {
    // Trigger a note first
    MIDIMessage noteOn(MIDIMessageType::NOTE_ON, 0, 60, 100, 0);
    midiHandler->processMIDIMessage(noteOn);
    
    // Create a note-off message
    MIDIMessage noteOff(MIDIMessageType::NOTE_OFF, 0, 60, 0, 0);
    
    // Process the message
    EXPECT_NO_THROW(midiHandler->processMIDIMessage(noteOff));
}

TEST_F(MIDIHandlerTest, ProcessMIDIMessageNoteOnWithZeroVelocity) {
    // Note-on with velocity 0 should be treated as note-off
    MIDIMessage message(MIDIMessageType::NOTE_ON, 0, 60, 0, 0);
    
    // This should be treated as note-off (isNoteOn() returns false)
    midiHandler->processMIDIMessage(message);
    
    // No voice should be allocated
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 0);
}

TEST_F(MIDIHandlerTest, ProcessMIDIMessageIgnoresCC) {
    // Create a CC message
    MIDIMessage message(MIDIMessageType::CC, 0, 7, 100, 0);
    
    // Process the message (should not crash, but won't do anything without mapping)
    EXPECT_NO_THROW(midiHandler->processMIDIMessage(message));
    
    // No voices should be allocated
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 0);
}

TEST_F(MIDIHandlerTest, ProcessMIDIMessageIgnoresPitchBend) {
    // Create a pitch bend message
    MIDIMessage message(MIDIMessageType::PITCH_BEND, 0, 0, 64, 0);
    
    // Process the message (should be ignored)
    EXPECT_NO_THROW(midiHandler->processMIDIMessage(message));
    
    // No voices should be allocated
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 0);
}

TEST_F(MIDIHandlerTest, ProcessMIDIMessageIgnoresUnknown) {
    // Create an unknown message
    MIDIMessage message(MIDIMessageType::UNKNOWN, 0, 0, 0, 0);
    
    // Process the message (should be ignored)
    EXPECT_NO_THROW(midiHandler->processMIDIMessage(message));
    
    // No voices should be allocated
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 0);
}

// ============================================================================
// Multiple Note Tests
// ============================================================================

TEST_F(MIDIHandlerTest, HandleMultipleNoteOns) {
    // Trigger multiple notes
    midiHandler->handleNoteOn(60, 100);
    midiHandler->handleNoteOn(64, 100);
    midiHandler->handleNoteOn(67, 100);
    
    // Three voices should be active
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 3);
}

TEST_F(MIDIHandlerTest, HandleNoteOnAndOffSequence) {
    // Trigger a note
    midiHandler->handleNoteOn(60, 100);
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
    
    // Release the note
    midiHandler->handleNoteOff(60);
    
    // Trigger another note
    midiHandler->handleNoteOn(64, 100);
    
    // At least one voice should be active
    EXPECT_GE(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandlePolyphony) {
    // Trigger 8 notes (max polyphony)
    for (int i = 0; i < 8; ++i) {
        midiHandler->handleNoteOn(60 + i, 100);
    }
    
    // All 8 voices should be active
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 8);
}

TEST_F(MIDIHandlerTest, HandleVoiceStealingOnPolyphonyLimit) {
    // Trigger 8 notes (max polyphony)
    for (int i = 0; i < 8; ++i) {
        midiHandler->handleNoteOn(60 + i, 100);
    }
    
    // Trigger one more note (should steal oldest voice)
    midiHandler->handleNoteOn(70, 100);
    
    // Still 8 voices active (one was stolen)
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 8);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(MIDIHandlerTest, HandleNoteOnWithNegativeVelocity) {
    // Negative velocity should be clamped to 0
    midiHandler->handleNoteOn(60, -10);
    
    // Voice should still be allocated
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandleNoteOnWithVelocityAbove127) {
    // Velocity above 127 should be clamped to 127
    midiHandler->handleNoteOn(60, 200);
    
    // Voice should still be allocated
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandleNoteOnWithMinNoteNumber) {
    // Minimum MIDI note number (0)
    midiHandler->handleNoteOn(0, 100);
    
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

TEST_F(MIDIHandlerTest, HandleNoteOnWithMaxNoteNumber) {
    // Maximum MIDI note number (127)
    midiHandler->handleNoteOn(127, 100);
    
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
}

// ============================================================================
// Voice Allocator Setter Tests
// ============================================================================

TEST_F(MIDIHandlerTest, SetVoiceAllocator) {
    VoiceAllocator newAllocator;
    newAllocator.initialize(48000.0f);
    
    midiHandler->setVoiceAllocator(&newAllocator);
    
    EXPECT_EQ(midiHandler->getVoiceAllocator(), &newAllocator);
}

TEST_F(MIDIHandlerTest, SetVoiceAllocatorToNull) {
    midiHandler->setVoiceAllocator(nullptr);
    
    EXPECT_EQ(midiHandler->getVoiceAllocator(), nullptr);
    
    // Should not crash when processing messages
    EXPECT_NO_THROW(midiHandler->handleNoteOn(60, 100));
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(MIDIHandlerTest, IntegrationTestNoteOnOffCycle) {
    // Simulate a complete note-on/off cycle
    MIDIMessage noteOn(MIDIMessageType::NOTE_ON, 0, 60, 100, 0);
    MIDIMessage noteOff(MIDIMessageType::NOTE_OFF, 0, 60, 0, 100);
    
    // Process note-on
    midiHandler->processMIDIMessage(noteOn);
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 1);
    
    // Process note-off
    midiHandler->processMIDIMessage(noteOff);
    
    // Voice should still be active (in release phase)
    // This is correct behavior - the envelope completes naturally
}

TEST_F(MIDIHandlerTest, IntegrationTestMultipleNotesWithDifferentVelocities) {
    // Trigger notes with different velocities
    midiHandler->handleNoteOn(60, 50);   // Soft
    midiHandler->handleNoteOn(64, 100);  // Medium
    midiHandler->handleNoteOn(67, 127);  // Hard
    
    // All three voices should be active
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 3);
    
    // Verify each voice has the correct note
    int noteCount[3] = {0, 0, 0};
    for (int i = 0; i < voiceAllocator.getNumVoices(); ++i) {
        const Voice& voice = voiceAllocator.getVoice(i);
        if (voice.isActive()) {
            int note = voice.getNote();
            if (note == 60) noteCount[0]++;
            else if (note == 64) noteCount[1]++;
            else if (note == 67) noteCount[2]++;
        }
    }
    
    EXPECT_EQ(noteCount[0], 1);
    EXPECT_EQ(noteCount[1], 1);
    EXPECT_EQ(noteCount[2], 1);
}

// ============================================================================
// CC Mapping Tests
// ============================================================================

TEST_F(MIDIHandlerTest, MapCCToParameter) {
    // Map CC 7 (volume) to master level
    bool result = midiHandler->mapCCToParameter(7, "masterLevel");
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(midiHandler->isCCMapped(7));
    EXPECT_EQ(midiHandler->getMappedParameter(7), "masterLevel");
}

TEST_F(MIDIHandlerTest, MapCCToNonExistentParameter) {
    // Try to map CC to a parameter that doesn't exist
    bool result = midiHandler->mapCCToParameter(7, "nonExistentParameter");
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(midiHandler->isCCMapped(7));
}

TEST_F(MIDIHandlerTest, MapCCWithInvalidCCNumber) {
    // Try to map invalid CC numbers
    EXPECT_FALSE(midiHandler->mapCCToParameter(-1, "masterLevel"));
    EXPECT_FALSE(midiHandler->mapCCToParameter(128, "masterLevel"));
}

TEST_F(MIDIHandlerTest, UnmapCC) {
    // Map and then unmap
    midiHandler->mapCCToParameter(7, "masterLevel");
    EXPECT_TRUE(midiHandler->isCCMapped(7));
    
    midiHandler->unmapCC(7);
    EXPECT_FALSE(midiHandler->isCCMapped(7));
    EXPECT_EQ(midiHandler->getMappedParameter(7), "");
}

TEST_F(MIDIHandlerTest, ClearAllCCMappings) {
    // Map multiple CCs
    midiHandler->mapCCToParameter(7, "masterLevel");
    midiHandler->mapCCToParameter(10, "basePitch");
    midiHandler->mapCCToParameter(11, "sineLevel");
    
    EXPECT_TRUE(midiHandler->isCCMapped(7));
    EXPECT_TRUE(midiHandler->isCCMapped(10));
    EXPECT_TRUE(midiHandler->isCCMapped(11));
    
    // Clear all mappings
    midiHandler->clearAllCCMappings();
    
    EXPECT_FALSE(midiHandler->isCCMapped(7));
    EXPECT_FALSE(midiHandler->isCCMapped(10));
    EXPECT_FALSE(midiHandler->isCCMapped(11));
}

TEST_F(MIDIHandlerTest, GetAllCCMappings) {
    // Map multiple CCs
    midiHandler->mapCCToParameter(7, "masterLevel");
    midiHandler->mapCCToParameter(10, "basePitch");
    
    auto mappings = midiHandler->getAllCCMappings();
    
    EXPECT_EQ(mappings.size(), 2);
    EXPECT_EQ(mappings[7], "masterLevel");
    EXPECT_EQ(mappings[10], "basePitch");
}

TEST_F(MIDIHandlerTest, HandleCCUpdatesParameter) {
    // Map CC 7 to master level
    midiHandler->mapCCToParameter(7, "masterLevel");
    
    // Get initial parameter value
    float initialValue = parameterManager->getParameterValue("masterLevel");
    
    // Send CC message with value 64 (middle)
    midiHandler->handleCC(7, 64);
    
    // Parameter should be updated (normalized to ~0.5)
    float newValue = parameterManager->getParameterValue("masterLevel");
    float expectedNormalized = 64.0f / 127.0f;
    float expectedValue = 0.0f + expectedNormalized * (100.0f - 0.0f); // masterLevel range is 0-100
    
    EXPECT_NEAR(newValue, expectedValue, 0.1f);
}

TEST_F(MIDIHandlerTest, HandleCCWithMinValue) {
    // Map CC to parameter
    midiHandler->mapCCToParameter(7, "masterLevel");
    
    // Send CC with minimum value (0)
    midiHandler->handleCC(7, 0);
    
    // Parameter should be at minimum
    float value = parameterManager->getParameterValue("masterLevel");
    EXPECT_NEAR(value, 0.0f, 0.1f);
}

TEST_F(MIDIHandlerTest, HandleCCWithMaxValue) {
    // Map CC to parameter
    midiHandler->mapCCToParameter(7, "masterLevel");
    
    // Send CC with maximum value (127)
    midiHandler->handleCC(7, 127);
    
    // Parameter should be at maximum
    float value = parameterManager->getParameterValue("masterLevel");
    EXPECT_NEAR(value, 100.0f, 0.1f);
}

TEST_F(MIDIHandlerTest, HandleCCWithoutMapping) {
    // Send CC without mapping (should not crash)
    EXPECT_NO_THROW(midiHandler->handleCC(7, 64));
}

TEST_F(MIDIHandlerTest, HandleCCWithNullParameterManager) {
    // Create handler without parameter manager
    MIDIHandler handler(&voiceAllocator, nullptr);
    
    // Map CC (should succeed even without parameter manager)
    handler.mapCCToParameter(7, "masterLevel");
    
    // Handle CC (should not crash)
    EXPECT_NO_THROW(handler.handleCC(7, 64));
}

TEST_F(MIDIHandlerTest, ProcessMIDIMessageCC) {
    // Map CC to parameter
    midiHandler->mapCCToParameter(7, "masterLevel");
    
    // Create CC message
    MIDIMessage message(MIDIMessageType::CC, 0, 7, 64, 0);
    
    // Process the message
    midiHandler->processMIDIMessage(message);
    
    // Parameter should be updated
    float value = parameterManager->getParameterValue("masterLevel");
    float expectedNormalized = 64.0f / 127.0f;
    float expectedValue = 0.0f + expectedNormalized * (100.0f - 0.0f);
    
    EXPECT_NEAR(value, expectedValue, 0.1f);
}

// ============================================================================
// CC Learn Tests
// ============================================================================

TEST_F(MIDIHandlerTest, EnableCCLearn) {
    // Enable CC learn for master level
    bool result = midiHandler->enableCCLearn("masterLevel");
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(midiHandler->isCCLearnActive());
    EXPECT_EQ(midiHandler->getCCLearnParameter(), "masterLevel");
}

TEST_F(MIDIHandlerTest, EnableCCLearnForNonExistentParameter) {
    // Try to enable CC learn for non-existent parameter
    bool result = midiHandler->enableCCLearn("nonExistentParameter");
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(midiHandler->isCCLearnActive());
}

TEST_F(MIDIHandlerTest, DisableCCLearn) {
    // Enable and then disable CC learn
    midiHandler->enableCCLearn("masterLevel");
    EXPECT_TRUE(midiHandler->isCCLearnActive());
    
    midiHandler->disableCCLearn();
    EXPECT_FALSE(midiHandler->isCCLearnActive());
    EXPECT_EQ(midiHandler->getCCLearnParameter(), "");
}

TEST_F(MIDIHandlerTest, CCLearnMapsController) {
    // Enable CC learn for master level
    midiHandler->enableCCLearn("masterLevel");
    
    // Send CC message (should map CC 7 to masterLevel)
    midiHandler->handleCC(7, 64);
    
    // CC learn should be disabled
    EXPECT_FALSE(midiHandler->isCCLearnActive());
    
    // CC 7 should now be mapped to masterLevel
    EXPECT_TRUE(midiHandler->isCCMapped(7));
    EXPECT_EQ(midiHandler->getMappedParameter(7), "masterLevel");
    
    // Parameter should also be updated with the CC value
    float value = parameterManager->getParameterValue("masterLevel");
    float expectedNormalized = 64.0f / 127.0f;
    float expectedValue = 0.0f + expectedNormalized * (100.0f - 0.0f);
    EXPECT_NEAR(value, expectedValue, 0.1f);
}

TEST_F(MIDIHandlerTest, CCLearnOverwritesExistingMapping) {
    // Map CC 7 to basePitch
    midiHandler->mapCCToParameter(7, "basePitch");
    
    // Enable CC learn for masterLevel
    midiHandler->enableCCLearn("masterLevel");
    
    // Send CC 7 (should remap to masterLevel)
    midiHandler->handleCC(7, 64);
    
    // CC 7 should now be mapped to masterLevel
    EXPECT_EQ(midiHandler->getMappedParameter(7), "masterLevel");
}

TEST_F(MIDIHandlerTest, CCLearnWithNullParameterManager) {
    // Create handler without parameter manager
    MIDIHandler handler(&voiceAllocator, nullptr);
    
    // Enable CC learn (should succeed even without parameter manager)
    bool result = handler.enableCCLearn("masterLevel");
    EXPECT_TRUE(result);
    
    // Handle CC (should map and disable learn mode)
    handler.handleCC(7, 64);
    
    EXPECT_FALSE(handler.isCCLearnActive());
    EXPECT_TRUE(handler.isCCMapped(7));
}

// ============================================================================
// Parameter Manager Setter Tests
// ============================================================================

TEST_F(MIDIHandlerTest, SetParameterManager) {
    ParameterManager newManager;
    newManager.registerAllSynthesisParameters();
    
    midiHandler->setParameterManager(&newManager);
    
    EXPECT_EQ(midiHandler->getParameterManager(), &newManager);
}

TEST_F(MIDIHandlerTest, SetParameterManagerToNull) {
    midiHandler->setParameterManager(nullptr);
    
    EXPECT_EQ(midiHandler->getParameterManager(), nullptr);
    
    // Should not crash when handling CC
    midiHandler->mapCCToParameter(7, "masterLevel");
    EXPECT_NO_THROW(midiHandler->handleCC(7, 64));
}

// ============================================================================
// Integration Tests with CC
// ============================================================================

TEST_F(MIDIHandlerTest, IntegrationTestCCControlsMultipleParameters) {
    // Map multiple CCs to different parameters
    midiHandler->mapCCToParameter(7, "masterLevel");
    midiHandler->mapCCToParameter(10, "basePitch");
    midiHandler->mapCCToParameter(11, "sineLevel");
    
    // Send CC messages
    midiHandler->handleCC(7, 100);
    midiHandler->handleCC(10, 64);
    midiHandler->handleCC(11, 80);
    
    // Verify all parameters were updated
    EXPECT_GT(parameterManager->getParameterValue("masterLevel"), 70.0f);
    EXPECT_GT(parameterManager->getParameterValue("basePitch"), 100.0f);
    EXPECT_GT(parameterManager->getParameterValue("sineLevel"), 60.0f);
}

TEST_F(MIDIHandlerTest, IntegrationTestCCLearnWorkflow) {
    // Simulate a typical CC learn workflow
    
    // 1. User enables CC learn for a parameter
    midiHandler->enableCCLearn("masterLevel");
    EXPECT_TRUE(midiHandler->isCCLearnActive());
    
    // 2. User moves a controller (CC 7)
    midiHandler->handleCC(7, 64);
    
    // 3. CC learn should be disabled and mapping created
    EXPECT_FALSE(midiHandler->isCCLearnActive());
    EXPECT_TRUE(midiHandler->isCCMapped(7));
    
    // 4. Future CC messages should control the parameter
    midiHandler->handleCC(7, 127);
    EXPECT_NEAR(parameterManager->getParameterValue("masterLevel"), 100.0f, 0.1f);
    
    midiHandler->handleCC(7, 0);
    EXPECT_NEAR(parameterManager->getParameterValue("masterLevel"), 0.0f, 0.1f);
}

// ============================================================================
// Pitch Bend Tests
// ============================================================================

TEST_F(MIDIHandlerTest, HandlePitchBendCenterValue) {
    // Trigger a note first
    midiHandler->handleNoteOn(60, 100);
    
    // Send pitch bend center value (8192 = no bend)
    // LSB = 0, MSB = 64 (8192 = 64 << 7 | 0)
    midiHandler->handlePitchBend(0, 64);
    
    // Current pitch bend should be 0.0 (center)
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), 0.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, HandlePitchBendMaxUp) {
    // Trigger a note first
    midiHandler->handleNoteOn(60, 100);
    
    // Send pitch bend max up value (16383)
    // LSB = 127, MSB = 127 (16383 = 127 << 7 | 127)
    midiHandler->handlePitchBend(127, 127);
    
    // Current pitch bend should be 1.0 (max up)
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), 1.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, HandlePitchBendMaxDown) {
    // Trigger a note first
    midiHandler->handleNoteOn(60, 100);
    
    // Send pitch bend max down value (0)
    // LSB = 0, MSB = 0 (0 = 0 << 7 | 0)
    midiHandler->handlePitchBend(0, 0);
    
    // Current pitch bend should be -1.0 (max down)
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), -1.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, HandlePitchBendHalfUp) {
    // Trigger a note first
    midiHandler->handleNoteOn(60, 100);
    
    // Send pitch bend half up value (12288 = 8192 + 4096)
    // LSB = 0, MSB = 96 (12288 = 96 << 7 | 0)
    midiHandler->handlePitchBend(0, 96);
    
    // Current pitch bend should be 0.5 (half up)
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), 0.5f, 0.001f);
}

TEST_F(MIDIHandlerTest, HandlePitchBendHalfDown) {
    // Trigger a note first
    midiHandler->handleNoteOn(60, 100);
    
    // Send pitch bend half down value (4096 = 8192 - 4096)
    // LSB = 0, MSB = 32 (4096 = 32 << 7 | 0)
    midiHandler->handlePitchBend(0, 32);
    
    // Current pitch bend should be -0.5 (half down)
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), -0.5f, 0.001f);
}

TEST_F(MIDIHandlerTest, HandlePitchBendWithNullVoiceAllocator) {
    // Create handler with null voice allocator
    MIDIHandler handler(nullptr);
    
    // Should not crash
    EXPECT_NO_THROW(handler.handlePitchBend(0, 64));
}

TEST_F(MIDIHandlerTest, HandlePitchBendAppliedToAllActiveVoices) {
    // Trigger multiple notes
    midiHandler->handleNoteOn(60, 100);
    midiHandler->handleNoteOn(64, 100);
    midiHandler->handleNoteOn(67, 100);
    
    EXPECT_EQ(voiceAllocator.getNumActiveVoices(), 3);
    
    // Send pitch bend
    midiHandler->handlePitchBend(0, 96);  // Half up
    
    // All active voices should have pitch bend applied
    for (int i = 0; i < voiceAllocator.getNumVoices(); ++i) {
        const Voice& voice = voiceAllocator.getVoice(i);
        if (voice.isActive()) {
            EXPECT_NEAR(voice.getPitchBendValue(), 0.5f, 0.001f);
        }
    }
}

TEST_F(MIDIHandlerTest, ProcessMIDIMessagePitchBend) {
    // Trigger a note first
    midiHandler->handleNoteOn(60, 100);
    
    // Create a pitch bend message (center value)
    MIDIMessage message(MIDIMessageType::PITCH_BEND, 0, 0, 64, 0);
    
    // Process the message
    midiHandler->processMIDIMessage(message);
    
    // Current pitch bend should be 0.0 (center)
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), 0.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, SetPitchBendRange) {
    // Set pitch bend range to 12 semitones (one octave)
    midiHandler->setPitchBendRange(12.0f);
    
    EXPECT_NEAR(midiHandler->getPitchBendRange(), 12.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, SetPitchBendRangeClampMin) {
    // Try to set negative pitch bend range (should be clamped to 0)
    midiHandler->setPitchBendRange(-5.0f);
    
    EXPECT_NEAR(midiHandler->getPitchBendRange(), 0.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, SetPitchBendRangeClampMax) {
    // Try to set pitch bend range above 24 semitones (should be clamped to 24)
    midiHandler->setPitchBendRange(30.0f);
    
    EXPECT_NEAR(midiHandler->getPitchBendRange(), 24.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, DefaultPitchBendRange) {
    // Default pitch bend range should be 2 semitones
    EXPECT_NEAR(midiHandler->getPitchBendRange(), 2.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, DefaultPitchBendValue) {
    // Default pitch bend value should be 0.0 (center)
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), 0.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, PitchBendRangeAppliedToVoices) {
    // Set pitch bend range to 12 semitones
    midiHandler->setPitchBendRange(12.0f);
    
    // Trigger a note
    midiHandler->handleNoteOn(60, 100);
    
    // Send pitch bend
    midiHandler->handlePitchBend(0, 96);  // Half up
    
    // Find the active voice and check its pitch bend range
    for (int i = 0; i < voiceAllocator.getNumVoices(); ++i) {
        const Voice& voice = voiceAllocator.getVoice(i);
        if (voice.isActive()) {
            EXPECT_NEAR(voice.getPitchBendRange(), 12.0f, 0.001f);
            return;
        }
    }
    
    FAIL() << "No active voice found";
}

// ============================================================================
// Integration Tests with Pitch Bend
// ============================================================================

TEST_F(MIDIHandlerTest, IntegrationTestPitchBendSequence) {
    // Trigger a note
    midiHandler->handleNoteOn(60, 100);
    
    // Send pitch bend center
    midiHandler->handlePitchBend(0, 64);
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), 0.0f, 0.001f);
    
    // Send pitch bend up
    midiHandler->handlePitchBend(127, 127);
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), 1.0f, 0.001f);
    
    // Send pitch bend down
    midiHandler->handlePitchBend(0, 0);
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), -1.0f, 0.001f);
    
    // Return to center
    midiHandler->handlePitchBend(0, 64);
    EXPECT_NEAR(midiHandler->getCurrentPitchBend(), 0.0f, 0.001f);
}

TEST_F(MIDIHandlerTest, IntegrationTestPitchBendWithMultipleNotes) {
    // Trigger multiple notes
    midiHandler->handleNoteOn(60, 100);
    midiHandler->handleNoteOn(64, 100);
    midiHandler->handleNoteOn(67, 100);
    
    // Send pitch bend
    midiHandler->handlePitchBend(0, 96);  // Half up
    
    // All voices should have the same pitch bend
    int activeVoiceCount = 0;
    for (int i = 0; i < voiceAllocator.getNumVoices(); ++i) {
        const Voice& voice = voiceAllocator.getVoice(i);
        if (voice.isActive()) {
            EXPECT_NEAR(voice.getPitchBendValue(), 0.5f, 0.001f);
            activeVoiceCount++;
        }
    }
    
    EXPECT_EQ(activeVoiceCount, 3);
}

TEST_F(MIDIHandlerTest, IntegrationTestPitchBendWithNoteOnOff) {
    // Trigger a note
    midiHandler->handleNoteOn(60, 100);
    
    // Apply pitch bend
    midiHandler->handlePitchBend(0, 96);  // Half up
    
    // Release the note
    midiHandler->handleNoteOff(60);
    
    // Trigger a new note (should inherit the current pitch bend)
    midiHandler->handleNoteOn(64, 100);
    
    // Find the new voice and check its pitch bend
    for (int i = 0; i < voiceAllocator.getNumVoices(); ++i) {
        const Voice& voice = voiceAllocator.getVoice(i);
        if (voice.isActive() && voice.getNote() == 64) {
            EXPECT_NEAR(voice.getPitchBendValue(), 0.5f, 0.001f);
            return;
        }
    }
}

TEST_F(MIDIHandlerTest, IntegrationTestPitchBendWithDifferentRanges) {
    // Test with default range (2 semitones)
    midiHandler->handleNoteOn(60, 100);
    midiHandler->handlePitchBend(127, 127);  // Max up
    
    // Change range to 12 semitones
    midiHandler->setPitchBendRange(12.0f);
    midiHandler->handleNoteOn(64, 100);
    midiHandler->handlePitchBend(127, 127);  // Max up
    
    // Both voices should have the same pitch bend value (1.0)
    // but different ranges
    int voiceCount = 0;
    for (int i = 0; i < voiceAllocator.getNumVoices(); ++i) {
        const Voice& voice = voiceAllocator.getVoice(i);
        if (voice.isActive()) {
            EXPECT_NEAR(voice.getPitchBendValue(), 1.0f, 0.001f);
            voiceCount++;
        }
    }
    
    EXPECT_EQ(voiceCount, 2);
}
