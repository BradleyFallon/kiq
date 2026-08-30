#include <gtest/gtest.h>
#include "audio_engine/effects/Reverb.h"
#include <cmath>
#include <vector>

using namespace KickDrum;

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(ReverbTest, DefaultConstruction) {
    Reverb reverb;
    
    // Should not be initialized yet
    EXPECT_FALSE(reverb.isInitialized());
    
    // Default parameters should be set
    EXPECT_FLOAT_EQ(reverb.getRoomSize(), 0.5f);
    EXPECT_FLOAT_EQ(reverb.getDecayTime(), 1.0f);
    EXPECT_FLOAT_EQ(reverb.getDamping(), 0.5f);
    EXPECT_FLOAT_EQ(reverb.getMix(), 0.3f);
}

TEST(ReverbTest, Initialization) {
    Reverb reverb;
    
    reverb.initialize(48000.0f);
    
    EXPECT_TRUE(reverb.isInitialized());
}

TEST(ReverbTest, InitializationWithInvalidSampleRate) {
    Reverb reverb;
    
    reverb.initialize(0.0f);
    EXPECT_FALSE(reverb.isInitialized());
    
    reverb.initialize(-48000.0f);
    EXPECT_FALSE(reverb.isInitialized());
}

// ============================================================================
// Parameter Tests
// ============================================================================

TEST(ReverbTest, SetRoomSize) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    reverb.setRoomSize(0.7f);
    EXPECT_FLOAT_EQ(reverb.getRoomSize(), 0.7f);
    
    // Test clamping
    reverb.setRoomSize(1.5f);
    EXPECT_FLOAT_EQ(reverb.getRoomSize(), 1.0f);
    
    reverb.setRoomSize(-0.5f);
    EXPECT_FLOAT_EQ(reverb.getRoomSize(), 0.0f);
}

TEST(ReverbTest, SetDecayTime) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    reverb.setDecayTime(2.5f);
    EXPECT_FLOAT_EQ(reverb.getDecayTime(), 2.5f);
    
    // Test clamping
    reverb.setDecayTime(15.0f);
    EXPECT_FLOAT_EQ(reverb.getDecayTime(), 10.0f);
    
    reverb.setDecayTime(0.05f);
    EXPECT_FLOAT_EQ(reverb.getDecayTime(), 0.1f);
}

TEST(ReverbTest, SetDamping) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    reverb.setDamping(0.8f);
    EXPECT_FLOAT_EQ(reverb.getDamping(), 0.8f);
    
    // Test clamping
    reverb.setDamping(1.5f);
    EXPECT_FLOAT_EQ(reverb.getDamping(), 1.0f);
    
    reverb.setDamping(-0.5f);
    EXPECT_FLOAT_EQ(reverb.getDamping(), 0.0f);
}

TEST(ReverbTest, SetMix) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    reverb.setMix(0.6f);
    EXPECT_FLOAT_EQ(reverb.getMix(), 0.6f);
    
    // Test clamping
    reverb.setMix(1.5f);
    EXPECT_FLOAT_EQ(reverb.getMix(), 1.0f);
    
    reverb.setMix(-0.5f);
    EXPECT_FLOAT_EQ(reverb.getMix(), 0.0f);
}

// ============================================================================
// Processing Tests
// ============================================================================

TEST(ReverbTest, BypassWhenNotInitialized) {
    Reverb reverb;
    
    float input = 0.5f;
    float output = reverb.process(input);
    
    // Should pass through unchanged
    EXPECT_FLOAT_EQ(output, input);
}

TEST(ReverbTest, ProcessSilence) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    // Process silence
    for (int i = 0; i < 1000; i++) {
        float output = reverb.process(0.0f);
        EXPECT_FLOAT_EQ(output, 0.0f);
    }
}

TEST(ReverbTest, DrySignalWithZeroMix) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(0.0f);  // Fully dry
    
    float input = 0.5f;
    float output = reverb.process(input);
    
    // With 0% mix, output should equal input
    EXPECT_FLOAT_EQ(output, input);
}

TEST(ReverbTest, WetSignalWithFullMix) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(1.0f);  // Fully wet
    reverb.setRoomSize(0.7f);
    
    // Send an impulse and collect outputs
    std::vector<float> outputs;
    outputs.push_back(reverb.process(1.0f));
    
    // Continue processing silence
    for (int i = 0; i < 2000; i++) {
        outputs.push_back(reverb.process(0.0f));
    }
    
    // Check if any output is non-zero (reverb response exists)
    bool hasNonZeroOutput = false;
    for (float output : outputs) {
        if (std::abs(output) > 0.0f) {
            hasNonZeroOutput = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasNonZeroOutput);
}

TEST(ReverbTest, ReverbTailDecays) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(1.0f);
    reverb.setDecayTime(0.3f);  // Short decay for clear test
    
    // Send an impulse
    reverb.process(1.0f);
    
    // Process silence and collect outputs
    std::vector<float> outputs;
    for (int i = 0; i < 15000; i++) {
        outputs.push_back(reverb.process(0.0f));
    }
    
    // Find peak in middle part (after initial delay, before significant decay)
    float middlePeak = 0.0f;
    for (int i = 100; i < 3000; i++) {
        middlePeak = std::max(middlePeak, std::abs(outputs[i]));
    }
    
    // Find peak in late part (after significant decay)
    float latePeak = 0.0f;
    for (int i = 12000; i < 15000; i++) {
        latePeak = std::max(latePeak, std::abs(outputs[i]));
    }
    
    // Late peak should be significantly smaller (reverb has decayed)
    EXPECT_GT(middlePeak, 0.0f);  // Ensure we have some reverb
    EXPECT_LT(latePeak, middlePeak * 0.5f);
}

TEST(ReverbTest, LongerDecayTimeProducesLongerTail) {
    Reverb reverb1;
    reverb1.initialize(48000.0f);
    reverb1.setMix(1.0f);
    reverb1.setDecayTime(0.5f);  // Short decay
    
    Reverb reverb2;
    reverb2.initialize(48000.0f);
    reverb2.setMix(1.0f);
    reverb2.setDecayTime(2.0f);  // Long decay
    
    // Send impulse to both
    reverb1.process(1.0f);
    reverb2.process(1.0f);
    
    // Process silence and measure when tail drops below threshold
    const float threshold = 0.001f;
    int samples1 = 0;
    int samples2 = 0;
    
    for (int i = 0; i < 10000; i++) {
        float out1 = reverb1.process(0.0f);
        if (std::abs(out1) > threshold) {
            samples1 = i;
        }
        
        float out2 = reverb2.process(0.0f);
        if (std::abs(out2) > threshold) {
            samples2 = i;
        }
    }
    
    // Longer decay time should produce longer tail
    EXPECT_GT(samples2, samples1);
}

TEST(ReverbTest, DampingAffectsHighFrequencies) {
    // This test verifies that damping affects the reverb character
    // Higher damping should reduce high-frequency content in the tail
    
    Reverb reverbLowDamp;
    reverbLowDamp.initialize(48000.0f);
    reverbLowDamp.setMix(1.0f);
    reverbLowDamp.setDamping(0.0f);  // No damping (bright)
    
    Reverb reverbHighDamp;
    reverbHighDamp.initialize(48000.0f);
    reverbHighDamp.setMix(1.0f);
    reverbHighDamp.setDamping(1.0f);  // Maximum damping (dark)
    
    // Send impulse to both
    reverbLowDamp.process(1.0f);
    reverbHighDamp.process(1.0f);
    
    // Process and collect outputs
    float sumLowDamp = 0.0f;
    float sumHighDamp = 0.0f;
    
    // The shortest comb delay is about 25 ms at 48 kHz, so measure far enough
    // into the response for damping to affect the feedback tail.
    for (int i = 0; i < 5000; i++) {
        sumLowDamp += std::abs(reverbLowDamp.process(0.0f));
        sumHighDamp += std::abs(reverbHighDamp.process(0.0f));
    }
    
    // Low damping should produce more energy in the tail
    // (high frequencies are preserved)
    EXPECT_GT(sumLowDamp, sumHighDamp);
}

TEST(ReverbTest, RoomSizeAffectsReverbCharacter) {
    Reverb reverbSmall;
    reverbSmall.initialize(48000.0f);
    reverbSmall.setMix(1.0f);
    reverbSmall.setRoomSize(0.1f);  // Small room
    
    Reverb reverbLarge;
    reverbLarge.initialize(48000.0f);
    reverbLarge.setMix(1.0f);
    reverbLarge.setRoomSize(0.9f);  // Large room
    
    // Send impulse to both
    reverbSmall.process(1.0f);
    reverbLarge.process(1.0f);
    
    // Measure tail length
    int tailSmall = 0;
    int tailLarge = 0;
    const float threshold = 0.001f;
    
    for (int i = 0; i < 10000; i++) {
        if (std::abs(reverbSmall.process(0.0f)) > threshold) {
            tailSmall = i;
        }
        if (std::abs(reverbLarge.process(0.0f)) > threshold) {
            tailLarge = i;
        }
    }
    
    // Larger room should have longer tail
    EXPECT_GT(tailLarge, tailSmall);
}

TEST(ReverbTest, MixBlendsDryAndWet) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    float input = 0.5f;
    
    // Test 0% mix (fully dry)
    reverb.setMix(0.0f);
    float output0 = reverb.process(input);
    EXPECT_FLOAT_EQ(output0, input);
    
    // Reset for next test
    reverb.reset();
    
    // Test 50% mix
    reverb.setMix(0.5f);
    float output50 = reverb.process(input);
    // Output should be between dry and wet
    // (exact value depends on reverb algorithm, but should be non-zero)
    EXPECT_NE(output50, 0.0f);
    
    // Reset for next test
    reverb.reset();
    
    // Test 100% mix (fully wet)
    reverb.setMix(1.0f);
    float output100 = reverb.process(input);
    // Should be different from dry signal
    EXPECT_NE(output100, input);
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST(ReverbTest, ResetClearsTail) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(1.0f);
    
    // Send impulse to create reverb tail
    reverb.process(1.0f);
    
    // Process some samples to build up tail
    for (int i = 0; i < 100; i++) {
        reverb.process(0.0f);
    }
    
    // Reset
    reverb.reset();
    
    // After reset, processing silence should produce silence
    for (int i = 0; i < 100; i++) {
        float output = reverb.process(0.0f);
        EXPECT_FLOAT_EQ(output, 0.0f);
    }
}

// ============================================================================
// Stability Tests
// ============================================================================

TEST(ReverbTest, StableWithContinuousInput) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    reverb.setMix(1.0f);
    reverb.setRoomSize(0.9f);  // Large room (high feedback)
    
    // Process continuous signal
    bool stable = true;
    for (int i = 0; i < 10000; i++) {
        float input = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / 48000.0f);
        float output = reverb.process(input);
        
        // Check for instability (NaN, infinity, or excessive amplitude)
        if (std::isnan(output) || std::isinf(output) || std::abs(output) > 10.0f) {
            stable = false;
            break;
        }
    }
    
    EXPECT_TRUE(stable);
}

TEST(ReverbTest, NoNaNOrInfinity) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    // Test with various extreme inputs
    std::vector<float> testInputs = {0.0f, 1.0f, -1.0f, 0.999f, -0.999f};
    
    for (float input : testInputs) {
        float output = reverb.process(input);
        EXPECT_FALSE(std::isnan(output));
        EXPECT_FALSE(std::isinf(output));
    }
}

// ============================================================================
// Multiple Sample Rate Tests
// ============================================================================

TEST(ReverbTest, WorksAtDifferentSampleRates) {
    std::vector<float> sampleRates = {44100.0f, 48000.0f, 88200.0f, 96000.0f};
    
    for (float sr : sampleRates) {
        Reverb reverb;
        reverb.initialize(sr);
        
        EXPECT_TRUE(reverb.isInitialized());
        
        // Process an impulse
        float output = reverb.process(1.0f);
        
        // Should produce valid output
        EXPECT_FALSE(std::isnan(output));
        EXPECT_FALSE(std::isinf(output));
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(ReverbTest, ExtremeMixValues) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    // Test with mix at boundaries
    reverb.setMix(0.0f);
    float output1 = reverb.process(0.5f);
    EXPECT_FALSE(std::isnan(output1));
    
    reverb.reset();
    reverb.setMix(1.0f);
    float output2 = reverb.process(0.5f);
    EXPECT_FALSE(std::isnan(output2));
}

TEST(ReverbTest, ExtremeRoomSizeValues) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    // Test with room size at boundaries
    reverb.setRoomSize(0.0f);
    float output1 = reverb.process(0.5f);
    EXPECT_FALSE(std::isnan(output1));
    
    reverb.reset();
    reverb.setRoomSize(1.0f);
    float output2 = reverb.process(0.5f);
    EXPECT_FALSE(std::isnan(output2));
}

TEST(ReverbTest, ExtremeDampingValues) {
    Reverb reverb;
    reverb.initialize(48000.0f);
    
    // Test with damping at boundaries
    reverb.setDamping(0.0f);
    float output1 = reverb.process(0.5f);
    EXPECT_FALSE(std::isnan(output1));
    
    reverb.reset();
    reverb.setDamping(1.0f);
    float output2 = reverb.process(0.5f);
    EXPECT_FALSE(std::isnan(output2));
}
