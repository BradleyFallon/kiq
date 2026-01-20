#include <gtest/gtest.h>
#include "audio_engine/voice/VoiceAllocator.h"
#include <cmath>

using namespace KickDrum;

class VoiceAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        allocator.initialize(44100.0f);
    }
    
    VoiceAllocator allocator;
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(VoiceAllocatorTest, InitializationCreatesEightVoices) {
    EXPECT_EQ(allocator.getNumVoices(), 8);
}

TEST_F(VoiceAllocatorTest, InitiallyNoActiveVoices) {
    EXPECT_EQ(allocator.getNumActiveVoices(), 0);
}

// ============================================================================
// Voice Allocation Tests
// ============================================================================

TEST_F(VoiceAllocatorTest, AllocateVoiceReturnsValidPointer) {
    Voice* voice = allocator.allocateVoice(60, 1.0f);
    ASSERT_NE(voice, nullptr);
}

TEST_F(VoiceAllocatorTest, AllocatedVoiceIsActive) {
    Voice* voice = allocator.allocateVoice(60, 1.0f);
    ASSERT_NE(voice, nullptr);
    EXPECT_TRUE(voice->isActive());
}

TEST_F(VoiceAllocatorTest, AllocatedVoiceHasCorrectNote) {
    Voice* voice = allocator.allocateVoice(60, 1.0f);
    ASSERT_NE(voice, nullptr);
    EXPECT_EQ(voice->getNote(), 60);
}

TEST_F(VoiceAllocatorTest, AllocatedVoiceAgeIsZero) {
    Voice* voice = allocator.allocateVoice(60, 1.0f);
    ASSERT_NE(voice, nullptr);
    EXPECT_EQ(voice->getAge(), 0);
}

TEST_F(VoiceAllocatorTest, MultipleAllocationsIncreaseActiveCount) {
    allocator.allocateVoice(60, 1.0f);
    EXPECT_EQ(allocator.getNumActiveVoices(), 1);
    
    allocator.allocateVoice(62, 1.0f);
    EXPECT_EQ(allocator.getNumActiveVoices(), 2);
    
    allocator.allocateVoice(64, 1.0f);
    EXPECT_EQ(allocator.getNumActiveVoices(), 3);
}

TEST_F(VoiceAllocatorTest, CanAllocateUpToEightVoices) {
    for (int i = 0; i < 8; ++i) {
        Voice* voice = allocator.allocateVoice(60 + i, 1.0f);
        ASSERT_NE(voice, nullptr);
    }
    EXPECT_EQ(allocator.getNumActiveVoices(), 8);
}

// ============================================================================
// Voice Stealing Tests
// ============================================================================

TEST_F(VoiceAllocatorTest, VoiceStealingWhenAllVoicesActive) {
    // Allocate all 8 voices
    for (int i = 0; i < 8; ++i) {
        allocator.allocateVoice(60 + i, 1.0f);
    }
    EXPECT_EQ(allocator.getNumActiveVoices(), 8);
    
    // Allocate a 9th voice - should steal the oldest
    Voice* voice = allocator.allocateVoice(70, 1.0f);
    ASSERT_NE(voice, nullptr);
    
    // Should still have 8 active voices
    EXPECT_EQ(allocator.getNumActiveVoices(), 8);
    
    // The new voice should be playing note 70
    EXPECT_EQ(voice->getNote(), 70);
}

TEST_F(VoiceAllocatorTest, VoiceStealingStealsOldestVoice) {
    // Allocate all 8 voices
    std::vector<Voice*> voices;
    for (int i = 0; i < 8; ++i) {
        voices.push_back(allocator.allocateVoice(60 + i, 1.0f));
    }
    
    // Render some samples to age the voices differently
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    // The first voice should now be the oldest
    Voice* oldestVoice = voices[0];
    uint64_t oldestAge = oldestVoice->getAge();
    
    // Allocate a 9th voice - should steal the oldest (first voice)
    Voice* newVoice = allocator.allocateVoice(70, 1.0f);
    ASSERT_NE(newVoice, nullptr);
    
    // The new voice should be the same pointer as the oldest voice
    EXPECT_EQ(newVoice, oldestVoice);
    
    // The new voice should have age 0 (just triggered)
    EXPECT_EQ(newVoice->getAge(), 0);
}

// ============================================================================
// Voice Release Tests
// ============================================================================

TEST_F(VoiceAllocatorTest, ReleaseVoiceByNote) {
    Voice* voice = allocator.allocateVoice(60, 1.0f);
    ASSERT_NE(voice, nullptr);
    EXPECT_TRUE(voice->isActive());
    
    allocator.releaseVoice(60);
    
    // Voice should still be active (in release phase)
    // but will eventually become inactive
    EXPECT_TRUE(voice->isActive());
}

TEST_F(VoiceAllocatorTest, ReleaseNonExistentNoteDoesNotCrash) {
    // Should not crash when releasing a note that's not playing
    allocator.releaseVoice(60);
    EXPECT_EQ(allocator.getNumActiveVoices(), 0);
}

TEST_F(VoiceAllocatorTest, ReleaseAllVoices) {
    // Allocate multiple voices
    for (int i = 0; i < 5; ++i) {
        allocator.allocateVoice(60 + i, 1.0f);
    }
    EXPECT_EQ(allocator.getNumActiveVoices(), 5);
    
    // Release all voices
    allocator.releaseAll();
    
    // All voices should still be active (in release phase)
    EXPECT_EQ(allocator.getNumActiveVoices(), 5);
}

// ============================================================================
// Audio Rendering Tests
// ============================================================================

TEST_F(VoiceAllocatorTest, RenderBufferWithNoActiveVoices) {
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    // Buffer should be all zeros
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(buffer[i], 0.0f);
    }
}

TEST_F(VoiceAllocatorTest, RenderBufferWithActiveVoice) {
    allocator.allocateVoice(60, 1.0f);
    
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    // Buffer should contain non-zero audio
    bool hasNonZero = false;
    for (int i = 0; i < 100; ++i) {
        if (buffer[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(VoiceAllocatorTest, RenderBufferMixesMultipleVoices) {
    // Allocate two voices
    allocator.allocateVoice(60, 1.0f);
    allocator.allocateVoice(64, 1.0f);
    
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    // Buffer should contain mixed audio from both voices
    bool hasNonZero = false;
    for (int i = 0; i < 100; ++i) {
        if (buffer[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(VoiceAllocatorTest, RenderBufferClearsBufferFirst) {
    float buffer[100];
    
    // Fill buffer with garbage
    for (int i = 0; i < 100; ++i) {
        buffer[i] = 999.0f;
    }
    
    // Render with no active voices
    allocator.renderBuffer(buffer, 100);
    
    // Buffer should be cleared to zero
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(buffer[i], 0.0f);
    }
}

TEST_F(VoiceAllocatorTest, RenderBufferProducesFiniteValues) {
    allocator.allocateVoice(60, 1.0f);
    
    float buffer[1000];
    allocator.renderBuffer(buffer, 1000);
    
    // All values should be finite (no NaN or infinity)
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(std::isfinite(buffer[i])) 
            << "Non-finite value at index " << i << ": " << buffer[i];
    }
}

// ============================================================================
// Voice Access Tests
// ============================================================================

TEST_F(VoiceAllocatorTest, GetVoiceByIndex) {
    for (int i = 0; i < 8; ++i) {
        Voice& voice = allocator.getVoice(i);
        // Should not crash and should return a valid reference
        EXPECT_FALSE(voice.isActive());
    }
}

TEST_F(VoiceAllocatorTest, GetNumActiveVoicesAccurate) {
    EXPECT_EQ(allocator.getNumActiveVoices(), 0);
    
    allocator.allocateVoice(60, 1.0f);
    EXPECT_EQ(allocator.getNumActiveVoices(), 1);
    
    allocator.allocateVoice(62, 1.0f);
    EXPECT_EQ(allocator.getNumActiveVoices(), 2);
    
    allocator.allocateVoice(64, 1.0f);
    EXPECT_EQ(allocator.getNumActiveVoices(), 3);
}

// ============================================================================
// Sample Rate Tests
// ============================================================================

TEST_F(VoiceAllocatorTest, SetSampleRateUpdatesAllVoices) {
    // Allocate a voice
    Voice* voice = allocator.allocateVoice(60, 1.0f);
    ASSERT_NE(voice, nullptr);
    
    // Change sample rate
    allocator.setSampleRate(48000.0f);
    
    // Voice should still work after sample rate change
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    // Should produce non-zero audio
    bool hasNonZero = false;
    for (int i = 0; i < 100; ++i) {
        if (buffer[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(VoiceAllocatorTest, AllocateWithZeroVelocity) {
    Voice* voice = allocator.allocateVoice(60, 0.0f);
    ASSERT_NE(voice, nullptr);
    EXPECT_TRUE(voice->isActive());
    
    // Should produce very quiet or zero output
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    // All values should be very small or zero
    for (int i = 0; i < 100; ++i) {
        EXPECT_NEAR(buffer[i], 0.0f, 0.01f);
    }
}

TEST_F(VoiceAllocatorTest, AllocateSameNoteTwice) {
    Voice* voice1 = allocator.allocateVoice(60, 1.0f);
    Voice* voice2 = allocator.allocateVoice(60, 1.0f);
    
    // Should allocate two different voices
    EXPECT_NE(voice1, voice2);
    
    // Both should be active
    EXPECT_TRUE(voice1->isActive());
    EXPECT_TRUE(voice2->isActive());
    
    // Both should have the same note
    EXPECT_EQ(voice1->getNote(), 60);
    EXPECT_EQ(voice2->getNote(), 60);
}

TEST_F(VoiceAllocatorTest, RenderWithNullBufferDoesNotCrash) {
    allocator.allocateVoice(60, 1.0f);
    
    // Should not crash with null buffer
    allocator.renderBuffer(nullptr, 100);
}

TEST_F(VoiceAllocatorTest, RenderWithZeroSamplesDoesNotCrash) {
    allocator.allocateVoice(60, 1.0f);
    
    float buffer[100];
    // Should not crash with zero samples
    allocator.renderBuffer(buffer, 0);
}

TEST_F(VoiceAllocatorTest, RenderWithNegativeSamplesDoesNotCrash) {
    allocator.allocateVoice(60, 1.0f);
    
    float buffer[100];
    // Should not crash with negative samples
    allocator.renderBuffer(buffer, -100);
}

// ============================================================================
// Polyphony Tests (Requirement 12.4)
// ============================================================================

TEST_F(VoiceAllocatorTest, SupportsEightSimultaneousVoices) {
    // Allocate 8 voices
    for (int i = 0; i < 8; ++i) {
        Voice* voice = allocator.allocateVoice(60 + i, 1.0f);
        ASSERT_NE(voice, nullptr);
        EXPECT_TRUE(voice->isActive());
    }
    
    // Should have exactly 8 active voices
    EXPECT_EQ(allocator.getNumActiveVoices(), 8);
    
    // Render audio with all 8 voices
    float buffer[100];
    allocator.renderBuffer(buffer, 100);
    
    // Should produce non-zero audio
    bool hasNonZero = false;
    for (int i = 0; i < 100; ++i) {
        if (buffer[i] != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(VoiceAllocatorTest, VoiceStealingMaintainsPolyphonyLimit) {
    // Allocate 10 voices (more than the limit)
    for (int i = 0; i < 10; ++i) {
        Voice* voice = allocator.allocateVoice(60 + i, 1.0f);
        ASSERT_NE(voice, nullptr);
        
        // Render a few samples to age the voices
        float buffer[10];
        allocator.renderBuffer(buffer, 10);
    }
    
    // Should never exceed 8 active voices
    EXPECT_LE(allocator.getNumActiveVoices(), 8);
}
