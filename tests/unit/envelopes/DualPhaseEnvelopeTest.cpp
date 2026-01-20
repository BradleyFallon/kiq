#include <gtest/gtest.h>
#include "audio_engine/envelopes/DualPhaseEnvelope.h"
#include <cmath>

using namespace KickDrum;

// Test fixture for DualPhaseEnvelope
class DualPhaseEnvelopeTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr float EPSILON = 0.001f;
    
    DualPhaseEnvelope* envelope;
    
    void SetUp() override {
        envelope = new DualPhaseEnvelope(SAMPLE_RATE);
    }
    
    void TearDown() override {
        delete envelope;
    }
    
    // Helper to advance envelope by a specific time in seconds
    void advanceByTime(float seconds) {
        int samples = static_cast<int>(seconds * SAMPLE_RATE);
        for (int i = 0; i < samples; ++i) {
            envelope->advance();
        }
    }
    
    // Helper to check if two floats are approximately equal
    bool approxEqual(float a, float b, float epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }
};

// ============================================================================
// Warm-Up Phase Parameter Tests
// ============================================================================

TEST_F(DualPhaseEnvelopeTest, WarmUpDurationDefaultValue) {
    EXPECT_FLOAT_EQ(envelope->getWarmUpDuration(), 0.0f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpDurationSetAndGet) {
    envelope->setWarmUpDuration(0.02f); // 20ms
    EXPECT_FLOAT_EQ(envelope->getWarmUpDuration(), 0.02f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpDurationClampsToMinimum) {
    envelope->setWarmUpDuration(-0.01f);
    EXPECT_FLOAT_EQ(envelope->getWarmUpDuration(), 0.0f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpDurationClampsToMaximum) {
    envelope->setWarmUpDuration(0.2f); // 200ms, should clamp to 100ms
    EXPECT_FLOAT_EQ(envelope->getWarmUpDuration(), 0.1f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpStartFrequencyDefaultValue) {
    EXPECT_FLOAT_EQ(envelope->getWarmUpStartFrequency(), 10.0f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpStartFrequencySetAndGet) {
    envelope->setWarmUpStartFrequency(20.0f);
    EXPECT_FLOAT_EQ(envelope->getWarmUpStartFrequency(), 20.0f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpStartFrequencyClampsToMinimum) {
    envelope->setWarmUpStartFrequency(2.0f); // Below 5Hz
    EXPECT_FLOAT_EQ(envelope->getWarmUpStartFrequency(), 5.0f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpStartFrequencyClampsToMaximum) {
    envelope->setWarmUpStartFrequency(100.0f); // Above 50Hz
    EXPECT_FLOAT_EQ(envelope->getWarmUpStartFrequency(), 50.0f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpAmplitudeDefaultValue) {
    EXPECT_FLOAT_EQ(envelope->getWarmUpAmplitude(), 0.5f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpAmplitudeSetAndGet) {
    envelope->setWarmUpAmplitude(0.7f);
    EXPECT_FLOAT_EQ(envelope->getWarmUpAmplitude(), 0.7f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpAmplitudeClampsToMinimum) {
    envelope->setWarmUpAmplitude(-0.1f);
    EXPECT_FLOAT_EQ(envelope->getWarmUpAmplitude(), 0.0f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpAmplitudeClampsToMaximum) {
    envelope->setWarmUpAmplitude(1.5f);
    EXPECT_FLOAT_EQ(envelope->getWarmUpAmplitude(), 1.0f);
}

// ============================================================================
// Warm-Up Phase Behavior Tests
// ============================================================================

TEST_F(DualPhaseEnvelopeTest, WarmUpPhaseBypassWhenDurationIsZero) {
    // Set warm-up duration to 0
    envelope->setWarmUpDuration(0.0f);
    
    // Trigger envelope
    envelope->trigger();
    
    // Should skip WARMUP and go directly to ATTACK
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::ATTACK);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpPhaseActiveWhenDurationIsNonZero) {
    // Set warm-up duration to 20ms
    envelope->setWarmUpDuration(0.02f);
    
    // Trigger envelope
    envelope->trigger();
    
    // Should enter WARMUP phase
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::WARMUP);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpPhaseStartsAtZero) {
    // Set warm-up parameters
    envelope->setWarmUpDuration(0.02f);
    envelope->setWarmUpAmplitude(0.8f);
    
    // Trigger envelope
    envelope->trigger();
    
    // Value should start at 0
    EXPECT_FLOAT_EQ(envelope->getValue(), 0.0f);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpPhaseBuildsToAmplitude) {
    // Set warm-up parameters
    envelope->setWarmUpDuration(0.02f); // 20ms
    envelope->setWarmUpAmplitude(0.8f);
    
    // Trigger envelope
    envelope->trigger();
    
    // Advance to middle of warm-up phase (10ms)
    advanceByTime(0.01f);
    
    // Should be around 0.4 (half of 0.8)
    EXPECT_TRUE(approxEqual(envelope->getValue(), 0.4f, 0.05f));
    
    // Advance to end of warm-up phase
    advanceByTime(0.01f);
    
    // Should be at warm-up amplitude
    EXPECT_TRUE(approxEqual(envelope->getValue(), 0.8f, 0.05f));
}

TEST_F(DualPhaseEnvelopeTest, WarmUpPhaseTransitionsToAttack) {
    // Set warm-up duration to 20ms
    envelope->setWarmUpDuration(0.02f);
    envelope->setAttack(0.01f); // 10ms attack
    
    // Trigger envelope
    envelope->trigger();
    
    // Should be in WARMUP
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::WARMUP);
    
    // Advance past warm-up duration
    advanceByTime(0.021f); // 21ms
    
    // Should have transitioned to ATTACK
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::ATTACK);
}

TEST_F(DualPhaseEnvelopeTest, WarmUpToAttackPhaseContinuity) {
    // Set warm-up duration
    envelope->setWarmUpDuration(0.02f);
    envelope->setWarmUpAmplitude(0.5f);
    envelope->setAttack(0.01f);
    
    // Trigger envelope
    envelope->trigger();
    
    // Advance to end of warm-up
    advanceByTime(0.02f);
    
    // Get value at end of warm-up
    float warmUpEndValue = envelope->getValue();
    
    // Advance one more sample to transition to attack
    envelope->advance();
    
    // Should be in ATTACK phase now
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::ATTACK);
    
    // Value should reset to 0 for attack phase (phase continuity)
    // This ensures the attack starts from 0, not from warm-up amplitude
    EXPECT_FLOAT_EQ(envelope->getValue(), 0.0f);
}

// ============================================================================
// ADSR Phase Tests
// ============================================================================

TEST_F(DualPhaseEnvelopeTest, ADSRParametersSetAndGet) {
    envelope->setAttack(0.01f);
    envelope->setDecay(0.5f);
    envelope->setSustain(0.3f);
    envelope->setRelease(0.2f);
    
    EXPECT_FLOAT_EQ(envelope->getAttack(), 0.01f);
    EXPECT_FLOAT_EQ(envelope->getDecay(), 0.5f);
    EXPECT_FLOAT_EQ(envelope->getSustain(), 0.3f);
    EXPECT_FLOAT_EQ(envelope->getRelease(), 0.2f);
}

TEST_F(DualPhaseEnvelopeTest, AttackPhaseReachesPeak) {
    // Set short attack
    envelope->setAttack(0.01f); // 10ms
    envelope->setWarmUpDuration(0.0f); // No warm-up
    
    // Trigger envelope
    envelope->trigger();
    
    // Advance through attack
    advanceByTime(0.011f);
    
    // Should be at or near peak (1.0)
    EXPECT_TRUE(approxEqual(envelope->getValue(), 1.0f, 0.05f));
}

TEST_F(DualPhaseEnvelopeTest, DecayPhaseTransitionsFromAttack) {
    envelope->setAttack(0.01f);
    envelope->setDecay(0.02f);
    envelope->setWarmUpDuration(0.0f);
    
    envelope->trigger();
    
    // Advance past attack
    advanceByTime(0.011f);
    
    // Should be in DECAY phase
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::DECAY);
}

TEST_F(DualPhaseEnvelopeTest, SustainPhaseHoldsLevel) {
    envelope->setAttack(0.01f);
    envelope->setDecay(0.02f);
    envelope->setSustain(0.3f);
    envelope->setWarmUpDuration(0.0f);
    
    envelope->trigger();
    
    // Advance past attack and decay
    advanceByTime(0.04f);
    
    // Should be in SUSTAIN phase
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::SUSTAIN);
    
    // Value should be at sustain level
    EXPECT_TRUE(approxEqual(envelope->getValue(), 0.3f, 0.05f));
}

TEST_F(DualPhaseEnvelopeTest, ZeroSustainAutoRelease) {
    // For kick drums, sustain is typically 0
    envelope->setAttack(0.01f);
    envelope->setDecay(0.02f);
    envelope->setSustain(0.0f);
    envelope->setRelease(0.01f);
    envelope->setWarmUpDuration(0.0f);
    
    envelope->trigger();
    
    // Advance past attack and decay
    advanceByTime(0.04f);
    
    // Should auto-transition to RELEASE when sustain is 0
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::RELEASE);
}

TEST_F(DualPhaseEnvelopeTest, ReleasePhaseReachesZero) {
    envelope->setAttack(0.01f);
    envelope->setDecay(0.02f);
    envelope->setSustain(0.0f);
    envelope->setRelease(0.01f);
    envelope->setWarmUpDuration(0.0f);
    
    envelope->trigger();
    
    // Advance through entire envelope
    advanceByTime(0.05f);
    
    // Should be in IDLE or near end of RELEASE
    EXPECT_TRUE(envelope->getCurrentPhase() == EnvelopePhase::IDLE ||
                envelope->getCurrentPhase() == EnvelopePhase::RELEASE);
    
    // Value should be at or near 0
    EXPECT_TRUE(envelope->getValue() < 0.1f);
}

// ============================================================================
// Curve Shaping Tests
// ============================================================================

TEST_F(DualPhaseEnvelopeTest, CurveTypesSetAndGet) {
    envelope->setAttackCurve(CurveType::EXPONENTIAL);
    envelope->setDecayCurve(CurveType::LOGARITHMIC);
    envelope->setReleaseCurve(CurveType::LINEAR);
    
    EXPECT_EQ(envelope->getAttackCurve(), CurveType::EXPONENTIAL);
    EXPECT_EQ(envelope->getDecayCurve(), CurveType::LOGARITHMIC);
    EXPECT_EQ(envelope->getReleaseCurve(), CurveType::LINEAR);
}

// ============================================================================
// State Management Tests
// ============================================================================

TEST_F(DualPhaseEnvelopeTest, InitialStateIsIdle) {
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::IDLE);
    EXPECT_FALSE(envelope->isActive());
    EXPECT_FLOAT_EQ(envelope->getValue(), 0.0f);
}

TEST_F(DualPhaseEnvelopeTest, TriggerActivatesEnvelope) {
    envelope->trigger();
    EXPECT_TRUE(envelope->isActive());
}

TEST_F(DualPhaseEnvelopeTest, ResetReturnsToIdle) {
    envelope->setWarmUpDuration(0.02f);
    envelope->trigger();
    
    // Advance a bit
    advanceByTime(0.01f);
    
    EXPECT_TRUE(envelope->isActive());
    
    // Reset
    envelope->reset();
    
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::IDLE);
    EXPECT_FALSE(envelope->isActive());
    EXPECT_FLOAT_EQ(envelope->getValue(), 0.0f);
}

TEST_F(DualPhaseEnvelopeTest, ReleaseTransitionsToReleasePhase) {
    envelope->setAttack(0.01f);
    envelope->setDecay(0.02f);
    envelope->setSustain(0.5f); // Non-zero sustain
    envelope->setWarmUpDuration(0.0f);
    
    envelope->trigger();
    
    // Advance to sustain phase
    advanceByTime(0.04f);
    
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::SUSTAIN);
    
    // Call release
    envelope->release();
    
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::RELEASE);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(DualPhaseEnvelopeTest, ZeroAttackTime) {
    envelope->setAttack(0.0f);
    envelope->setWarmUpDuration(0.0f);
    
    envelope->trigger();
    
    // Should immediately transition to DECAY
    envelope->advance();
    
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::DECAY);
}

TEST_F(DualPhaseEnvelopeTest, AllTimesZero) {
    envelope->setWarmUpDuration(0.0f);
    envelope->setAttack(0.0f);
    envelope->setDecay(0.0f);
    envelope->setSustain(0.0f);
    envelope->setRelease(0.0f);
    
    envelope->trigger();
    
    // Should quickly reach IDLE
    for (int i = 0; i < 10; ++i) {
        envelope->advance();
    }
    
    EXPECT_EQ(envelope->getCurrentPhase(), EnvelopePhase::IDLE);
}

TEST_F(DualPhaseEnvelopeTest, SampleRateChange) {
    envelope->setWarmUpDuration(0.02f);
    envelope->setSampleRate(96000.0f); // Double sample rate
    
    envelope->trigger();
    
    // With doubled sample rate, need twice as many samples for same duration
    int samples = static_cast<int>(0.02f * 96000.0f);
    for (int i = 0; i < samples + 100; ++i) {
        envelope->advance();
    }
    
    // Should have transitioned past warm-up
    EXPECT_NE(envelope->getCurrentPhase(), EnvelopePhase::WARMUP);
}
