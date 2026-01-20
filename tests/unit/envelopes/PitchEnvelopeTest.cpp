#include <gtest/gtest.h>
#include "audio_engine/envelopes/PitchEnvelope.h"
#include <cmath>

using namespace KickDrum;

// Test fixture for PitchEnvelope
class PitchEnvelopeTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr float EPSILON = 0.001f;
    
    PitchEnvelope* pitchEnvelope;
    
    void SetUp() override {
        pitchEnvelope = new PitchEnvelope(SAMPLE_RATE);
    }
    
    void TearDown() override {
        delete pitchEnvelope;
    }
    
    // Helper to advance envelope by a specific time in seconds
    void advanceByTime(float seconds) {
        int samples = static_cast<int>(seconds * SAMPLE_RATE);
        for (int i = 0; i < samples; ++i) {
            pitchEnvelope->advance();
        }
    }
    
    // Helper to check if two floats are approximately equal
    bool approxEqual(float a, float b, float epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }
};

// ============================================================================
// Depth Parameter Tests
// ============================================================================

TEST_F(PitchEnvelopeTest, DepthDefaultValue) {
    // Default depth should be 500Hz
    EXPECT_FLOAT_EQ(pitchEnvelope->getDepth(), 500.0f);
}

TEST_F(PitchEnvelopeTest, DepthSetAndGet) {
    pitchEnvelope->setDepth(1000.0f);
    EXPECT_FLOAT_EQ(pitchEnvelope->getDepth(), 1000.0f);
}

TEST_F(PitchEnvelopeTest, DepthClampsToMinimum) {
    pitchEnvelope->setDepth(-100.0f);
    EXPECT_FLOAT_EQ(pitchEnvelope->getDepth(), 0.0f);
}

TEST_F(PitchEnvelopeTest, DepthClampsToMaximum) {
    pitchEnvelope->setDepth(3000.0f); // Above 2000Hz
    EXPECT_FLOAT_EQ(pitchEnvelope->getDepth(), 2000.0f);
}

// ============================================================================
// Frequency Offset Tests
// ============================================================================

TEST_F(PitchEnvelopeTest, InitialValueIsZero) {
    // Before triggering, value should be 0
    EXPECT_FLOAT_EQ(pitchEnvelope->getValue(), 0.0f);
}

TEST_F(PitchEnvelopeTest, ValueIsZeroWhenDepthIsZero) {
    pitchEnvelope->setDepth(0.0f);
    pitchEnvelope->trigger();
    
    // Advance a bit
    advanceByTime(0.01f);
    
    // Value should always be 0 when depth is 0
    EXPECT_FLOAT_EQ(pitchEnvelope->getValue(), 0.0f);
}

TEST_F(PitchEnvelopeTest, ValueScalesWithDepth) {
    // Set depth to 1000Hz
    pitchEnvelope->setDepth(1000.0f);
    
    // Trigger envelope
    pitchEnvelope->trigger();
    
    // Advance to peak (after attack phase)
    advanceByTime(0.002f); // 2ms, past the 1ms attack
    
    // Value should be close to depth (1000Hz) at peak
    float value = pitchEnvelope->getValue();
    EXPECT_GT(value, 900.0f); // Should be near 1000Hz
    EXPECT_LE(value, 1000.0f); // Should not exceed depth
}

TEST_F(PitchEnvelopeTest, ValueDecaysToZero) {
    pitchEnvelope->setDepth(500.0f);
    pitchEnvelope->trigger();
    
    // Advance through entire envelope (attack + decay + release)
    advanceByTime(0.2f); // 200ms
    
    // Value should be at or near 0
    EXPECT_LT(pitchEnvelope->getValue(), 10.0f);
}

TEST_F(PitchEnvelopeTest, ValueRangeWithDifferentDepths) {
    // Test with minimum depth
    pitchEnvelope->setDepth(0.0f);
    pitchEnvelope->trigger();
    advanceByTime(0.002f);
    EXPECT_FLOAT_EQ(pitchEnvelope->getValue(), 0.0f);
    
    // Reset and test with maximum depth
    pitchEnvelope->reset();
    pitchEnvelope->setDepth(2000.0f);
    pitchEnvelope->trigger();
    advanceByTime(0.002f);
    float maxValue = pitchEnvelope->getValue();
    EXPECT_GT(maxValue, 1800.0f); // Should be near 2000Hz
    EXPECT_LE(maxValue, 2000.0f);
}

// ============================================================================
// Envelope Control Tests
// ============================================================================

TEST_F(PitchEnvelopeTest, TriggerActivatesEnvelope) {
    EXPECT_FALSE(pitchEnvelope->isActive());
    
    pitchEnvelope->trigger();
    
    EXPECT_TRUE(pitchEnvelope->isActive());
}

TEST_F(PitchEnvelopeTest, ResetDeactivatesEnvelope) {
    pitchEnvelope->trigger();
    EXPECT_TRUE(pitchEnvelope->isActive());
    
    pitchEnvelope->reset();
    
    EXPECT_FALSE(pitchEnvelope->isActive());
    EXPECT_FLOAT_EQ(pitchEnvelope->getValue(), 0.0f);
}

TEST_F(PitchEnvelopeTest, ReleaseTransitionsToReleasePhase) {
    pitchEnvelope->trigger();
    
    // Advance a bit
    advanceByTime(0.01f);
    
    EXPECT_TRUE(pitchEnvelope->isActive());
    
    // Release
    pitchEnvelope->release();
    
    // Should still be active but in release phase
    EXPECT_TRUE(pitchEnvelope->isActive());
    
    // Advance through release
    advanceByTime(0.1f);
    
    // Should eventually become inactive
    EXPECT_FALSE(pitchEnvelope->isActive());
}

TEST_F(PitchEnvelopeTest, EnvelopeBecomesInactiveAfterCompletion) {
    pitchEnvelope->trigger();
    
    // Advance through entire envelope
    advanceByTime(0.3f); // 300ms, well past attack + decay + release
    
    EXPECT_FALSE(pitchEnvelope->isActive());
}

// ============================================================================
// Underlying Envelope Access Tests
// ============================================================================

TEST_F(PitchEnvelopeTest, CanAccessUnderlyingEnvelope) {
    DualPhaseEnvelope& envelope = pitchEnvelope->getEnvelope();
    
    // Modify underlying envelope parameters
    envelope.setAttack(0.05f);
    
    EXPECT_FLOAT_EQ(envelope.getAttack(), 0.05f);
}

TEST_F(PitchEnvelopeTest, CanConfigureEnvelopeParameters) {
    // Configure the underlying envelope
    pitchEnvelope->getEnvelope().setAttack(0.01f);
    pitchEnvelope->getEnvelope().setDecay(0.2f);
    pitchEnvelope->getEnvelope().setSustain(0.0f);
    pitchEnvelope->getEnvelope().setRelease(0.1f);
    
    // Verify parameters were set
    EXPECT_FLOAT_EQ(pitchEnvelope->getEnvelope().getAttack(), 0.01f);
    EXPECT_FLOAT_EQ(pitchEnvelope->getEnvelope().getDecay(), 0.2f);
    EXPECT_FLOAT_EQ(pitchEnvelope->getEnvelope().getSustain(), 0.0f);
    EXPECT_FLOAT_EQ(pitchEnvelope->getEnvelope().getRelease(), 0.1f);
}

TEST_F(PitchEnvelopeTest, CanConfigureEnvelopeCurves) {
    // Configure curve types
    pitchEnvelope->getEnvelope().setAttackCurve(CurveType::LINEAR);
    pitchEnvelope->getEnvelope().setDecayCurve(CurveType::LOGARITHMIC);
    pitchEnvelope->getEnvelope().setReleaseCurve(CurveType::EXPONENTIAL);
    
    // Verify curves were set
    EXPECT_EQ(pitchEnvelope->getEnvelope().getAttackCurve(), CurveType::LINEAR);
    EXPECT_EQ(pitchEnvelope->getEnvelope().getDecayCurve(), CurveType::LOGARITHMIC);
    EXPECT_EQ(pitchEnvelope->getEnvelope().getReleaseCurve(), CurveType::EXPONENTIAL);
}

// ============================================================================
// Sample Rate Tests
// ============================================================================

TEST_F(PitchEnvelopeTest, SampleRateChange) {
    pitchEnvelope->setDepth(1000.0f);
    pitchEnvelope->setSampleRate(96000.0f); // Double sample rate
    
    pitchEnvelope->trigger();
    
    // With doubled sample rate, timing should still be correct
    // Advance 2ms at 96kHz
    int samples = static_cast<int>(0.002f * 96000.0f);
    for (int i = 0; i < samples; ++i) {
        pitchEnvelope->advance();
    }
    
    // Should be near peak
    float value = pitchEnvelope->getValue();
    EXPECT_GT(value, 900.0f);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(PitchEnvelopeTest, TypicalKickDrumPitchSweep) {
    // Configure for typical kick drum pitch sweep
    pitchEnvelope->setDepth(800.0f); // 800Hz sweep
    pitchEnvelope->getEnvelope().setAttack(0.001f);  // 1ms attack
    pitchEnvelope->getEnvelope().setDecay(0.08f);    // 80ms decay
    pitchEnvelope->getEnvelope().setSustain(0.0f);   // Decay to base pitch
    pitchEnvelope->getEnvelope().setRelease(0.02f);  // 20ms release
    
    pitchEnvelope->trigger();
    
    // At start, should be near 0
    EXPECT_LT(pitchEnvelope->getValue(), 50.0f);
    
    // After attack (2ms), should be near peak
    advanceByTime(0.002f);
    float peakValue = pitchEnvelope->getValue();
    EXPECT_GT(peakValue, 700.0f);
    EXPECT_LE(peakValue, 800.0f);
    
    // After 40ms (middle of decay), should be around half
    advanceByTime(0.038f); // Total 40ms
    float midValue = pitchEnvelope->getValue();
    EXPECT_GT(midValue, 100.0f);
    EXPECT_LT(midValue, 700.0f);
    
    // After full envelope (150ms), should be near 0
    advanceByTime(0.11f); // Total 150ms
    EXPECT_LT(pitchEnvelope->getValue(), 50.0f);
}

TEST_F(PitchEnvelopeTest, RetriggerBehavior) {
    pitchEnvelope->setDepth(500.0f);
    
    // First trigger
    pitchEnvelope->trigger();
    advanceByTime(0.05f); // 50ms
    
    float firstValue = pitchEnvelope->getValue();
    EXPECT_GT(firstValue, 0.0f);
    
    // Retrigger
    pitchEnvelope->trigger();
    
    // Should restart from beginning
    float retriggeredValue = pitchEnvelope->getValue();
    EXPECT_LT(retriggeredValue, firstValue);
    
    // After advancing, should reach peak again
    advanceByTime(0.002f);
    float newPeakValue = pitchEnvelope->getValue();
    EXPECT_GT(newPeakValue, 400.0f);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(PitchEnvelopeTest, ZeroDepthProducesZeroOffset) {
    pitchEnvelope->setDepth(0.0f);
    pitchEnvelope->trigger();
    
    // Advance through various points in the envelope
    for (int i = 0; i < 10; ++i) {
        advanceByTime(0.01f);
        EXPECT_FLOAT_EQ(pitchEnvelope->getValue(), 0.0f);
    }
}

TEST_F(PitchEnvelopeTest, MaximumDepthProducesCorrectRange) {
    pitchEnvelope->setDepth(2000.0f);
    pitchEnvelope->trigger();
    
    // Advance to peak
    advanceByTime(0.002f);
    
    float value = pitchEnvelope->getValue();
    EXPECT_GT(value, 1800.0f);
    EXPECT_LE(value, 2000.0f);
}

TEST_F(PitchEnvelopeTest, MultipleTriggersWithoutReset) {
    pitchEnvelope->setDepth(500.0f);
    
    // Trigger multiple times
    for (int i = 0; i < 5; ++i) {
        pitchEnvelope->trigger();
        advanceByTime(0.002f);
        
        // Each trigger should produce similar peak values
        float value = pitchEnvelope->getValue();
        EXPECT_GT(value, 400.0f);
        EXPECT_LE(value, 500.0f);
        
        // Let it decay a bit before next trigger
        advanceByTime(0.01f);
    }
}
