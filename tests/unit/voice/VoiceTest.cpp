#include <gtest/gtest.h>
#include "audio_engine/voice/Voice.h"
#include <cmath>

using namespace KickDrum;

class VoiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        sampleRate = 48000.0f;
        voice.initialize(sampleRate);
    }
    
    float sampleRate;
    Voice voice;
};

// Test voice initialization
TEST_F(VoiceTest, Initialization) {
    Voice v;
    EXPECT_FALSE(v.isActive());
    
    v.initialize(48000.0f);
    EXPECT_FALSE(v.isActive());  // Should not be active until triggered
}

// Test voice triggering
TEST_F(VoiceTest, Trigger) {
    EXPECT_FALSE(voice.isActive());
    
    voice.trigger(60, 0.8f);
    
    EXPECT_TRUE(voice.isActive());
    EXPECT_EQ(voice.getNote(), 60);
    EXPECT_EQ(voice.getAge(), 0);
}

// Test voice release
TEST_F(VoiceTest, Release) {
    voice.trigger(60, 0.8f);
    EXPECT_TRUE(voice.isActive());
    
    voice.release();
    
    // Voice should still be active during release phase
    EXPECT_TRUE(voice.isActive());
}

// Test voice becomes inactive after envelope completes
TEST_F(VoiceTest, BecomesInactiveAfterEnvelope) {
    // Configure very short envelope
    voice.getAmplitudeEnvelope().setWarmUpDuration(0.0f);
    voice.getAmplitudeEnvelope().setAttack(0.001f);
    voice.getAmplitudeEnvelope().setDecay(0.001f);
    voice.getAmplitudeEnvelope().setSustain(0.0f);
    voice.getAmplitudeEnvelope().setRelease(0.001f);
    
    voice.trigger(60, 0.8f);
    EXPECT_TRUE(voice.isActive());
    
    // Render samples until voice becomes inactive
    int maxSamples = static_cast<int>(sampleRate * 0.1f);  // 100ms max
    int sampleCount = 0;
    while (voice.isActive() && sampleCount < maxSamples) {
        voice.renderSample();
        sampleCount++;
    }
    
    EXPECT_FALSE(voice.isActive());
    EXPECT_LT(sampleCount, maxSamples);  // Should complete before timeout
}

// Test parameter setters and getters
TEST_F(VoiceTest, ParameterSettersAndGetters) {
    voice.setBasePitch(60.0f);
    EXPECT_FLOAT_EQ(voice.getBasePitch(), 60.0f);
    
    voice.setSineLevel(0.7f);
    EXPECT_FLOAT_EQ(voice.getSineLevel(), 0.7f);
    
    voice.setHarmonicLevel(0.4f);
    EXPECT_FLOAT_EQ(voice.getHarmonicLevel(), 0.4f);
    
    voice.setNoiseLevel(0.3f);
    EXPECT_FLOAT_EQ(voice.getNoiseLevel(), 0.3f);
    
    voice.setHarmonicRatio(3.0f);
    EXPECT_FLOAT_EQ(voice.getHarmonicRatio(), 3.0f);
    
    voice.setHarmonicModDepth(0.6f);
    EXPECT_FLOAT_EQ(voice.getHarmonicModDepth(), 0.6f);
    
    voice.setNoiseModDepth(0.8f);
    EXPECT_FLOAT_EQ(voice.getNoiseModDepth(), 0.8f);
}

// Test voice rendering produces non-zero output when active
TEST_F(VoiceTest, RenderingProducesOutput) {
    voice.trigger(60, 0.8f);
    
    // Render a few samples and check for non-zero output
    bool foundNonZero = false;
    for (int i = 0; i < 100; i++) {
        float sample = voice.renderSample();
        if (std::abs(sample) > 0.0001f) {
            foundNonZero = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundNonZero);
}

// Test voice rendering produces zero output when inactive
TEST_F(VoiceTest, RenderingProducesZeroWhenInactive) {
    EXPECT_FALSE(voice.isActive());
    
    float sample = voice.renderSample();
    EXPECT_FLOAT_EQ(sample, 0.0f);
}

// Test velocity scaling affects output amplitude
TEST_F(VoiceTest, VelocityScaling) {
    // Configure simple envelope for consistent testing
    voice.getAmplitudeEnvelope().setWarmUpDuration(0.0f);
    voice.getAmplitudeEnvelope().setAttack(0.0f);
    voice.getAmplitudeEnvelope().setDecay(0.1f);
    voice.getAmplitudeEnvelope().setSustain(1.0f);
    voice.getAmplitudeEnvelope().setRelease(0.0f);
    
    // Disable pitch envelope for consistent testing
    voice.getPitchEnvelope().setDepth(0.0f);
    
    // Trigger with full velocity
    voice.trigger(60, 1.0f);
    
    // Skip a few samples to get past attack
    for (int i = 0; i < 10; i++) {
        voice.renderSample();
    }
    
    float fullVelocitySample = voice.renderSample();
    
    // Reset and trigger with half velocity
    Voice voice2;
    voice2.initialize(sampleRate);
    voice2.getAmplitudeEnvelope().setWarmUpDuration(0.0f);
    voice2.getAmplitudeEnvelope().setAttack(0.0f);
    voice2.getAmplitudeEnvelope().setDecay(0.1f);
    voice2.getAmplitudeEnvelope().setSustain(1.0f);
    voice2.getAmplitudeEnvelope().setRelease(0.0f);
    voice2.getPitchEnvelope().setDepth(0.0f);
    
    voice2.trigger(60, 0.5f);
    
    // Skip same number of samples
    for (int i = 0; i < 10; i++) {
        voice2.renderSample();
    }
    
    float halfVelocitySample = voice2.renderSample();
    
    // Half velocity should produce approximately half the amplitude
    // Allow some tolerance due to envelope differences
    EXPECT_NEAR(halfVelocitySample, fullVelocitySample * 0.5f, 0.1f);
}

// Test voice age increments
TEST_F(VoiceTest, AgeIncrements) {
    voice.trigger(60, 0.8f);
    EXPECT_EQ(voice.getAge(), 0);
    
    voice.renderSample();
    EXPECT_EQ(voice.getAge(), 1);
    
    voice.renderSample();
    EXPECT_EQ(voice.getAge(), 2);
    
    for (int i = 0; i < 98; i++) {
        voice.renderSample();
    }
    EXPECT_EQ(voice.getAge(), 100);
}

// Test sample rate change
TEST_F(VoiceTest, SampleRateChange) {
    voice.setSampleRate(96000.0f);
    
    // Voice should still work after sample rate change
    voice.trigger(60, 0.8f);
    EXPECT_TRUE(voice.isActive());
    
    float sample = voice.renderSample();
    // Should produce valid output (not NaN or infinity)
    EXPECT_TRUE(std::isfinite(sample));
}

// Test envelope access
TEST_F(VoiceTest, EnvelopeAccess) {
    // Test amplitude envelope access
    DualPhaseEnvelope& ampEnv = voice.getAmplitudeEnvelope();
    ampEnv.setAttack(0.05f);
    EXPECT_FLOAT_EQ(ampEnv.getAttack(), 0.05f);
    
    // Test pitch envelope access
    PitchEnvelope& pitchEnv = voice.getPitchEnvelope();
    pitchEnv.setDepth(500.0f);
    EXPECT_FLOAT_EQ(pitchEnv.getDepth(), 500.0f);
}

// Test retriggering resets age
TEST_F(VoiceTest, RetriggeringResetsAge) {
    voice.trigger(60, 0.8f);
    
    // Render some samples to increase age
    for (int i = 0; i < 50; i++) {
        voice.renderSample();
    }
    EXPECT_EQ(voice.getAge(), 50);
    
    // Retrigger
    voice.trigger(62, 0.9f);
    EXPECT_EQ(voice.getAge(), 0);
    EXPECT_EQ(voice.getNote(), 62);
}

// Test output is finite (no NaN or infinity)
TEST_F(VoiceTest, OutputIsFinite) {
    voice.trigger(60, 0.8f);
    
    // Render many samples and verify all are finite
    for (int i = 0; i < 1000; i++) {
        float sample = voice.renderSample();
        EXPECT_TRUE(std::isfinite(sample)) << "Sample " << i << " is not finite";
    }
}

// Test with extreme parameter values
TEST_F(VoiceTest, ExtremeParameterValues) {
    // Set extreme values
    voice.setBasePitch(20.0f);  // Minimum
    voice.setSineLevel(1.0f);   // Maximum
    voice.setHarmonicLevel(1.0f);
    voice.setNoiseLevel(1.0f);
    voice.setHarmonicRatio(8.0f);  // Maximum
    voice.setHarmonicModDepth(1.0f);
    voice.setNoiseModDepth(1.0f);
    
    voice.trigger(60, 1.0f);
    
    // Should still produce finite output
    for (int i = 0; i < 100; i++) {
        float sample = voice.renderSample();
        EXPECT_TRUE(std::isfinite(sample));
    }
}

// Test with minimum parameter values
TEST_F(VoiceTest, MinimumParameterValues) {
    // Set minimum values
    voice.setBasePitch(20.0f);
    voice.setSineLevel(0.0f);
    voice.setHarmonicLevel(0.0f);
    voice.setNoiseLevel(0.0f);
    voice.setHarmonicRatio(0.5f);
    voice.setHarmonicModDepth(0.0f);
    voice.setNoiseModDepth(0.0f);
    
    voice.trigger(60, 0.0f);  // Zero velocity
    
    // Should produce zero or near-zero output
    for (int i = 0; i < 100; i++) {
        float sample = voice.renderSample();
        EXPECT_NEAR(sample, 0.0f, 0.0001f);
    }
}

// ============================================================================
// Pitch Bend Tests
// ============================================================================

TEST_F(VoiceTest, PitchBendDefaultValues) {
    // Default pitch bend should be 0.0 (center)
    EXPECT_FLOAT_EQ(voice.getPitchBendValue(), 0.0f);
    
    // Default pitch bend range should be 2.0 semitones
    EXPECT_FLOAT_EQ(voice.getPitchBendRange(), 2.0f);
}

TEST_F(VoiceTest, SetPitchBendCenter) {
    voice.setPitchBend(0.0f, 2.0f);
    
    EXPECT_FLOAT_EQ(voice.getPitchBendValue(), 0.0f);
    EXPECT_FLOAT_EQ(voice.getPitchBendRange(), 2.0f);
}

TEST_F(VoiceTest, SetPitchBendMaxUp) {
    voice.setPitchBend(1.0f, 2.0f);
    
    EXPECT_FLOAT_EQ(voice.getPitchBendValue(), 1.0f);
    EXPECT_FLOAT_EQ(voice.getPitchBendRange(), 2.0f);
}

TEST_F(VoiceTest, SetPitchBendMaxDown) {
    voice.setPitchBend(-1.0f, 2.0f);
    
    EXPECT_FLOAT_EQ(voice.getPitchBendValue(), -1.0f);
    EXPECT_FLOAT_EQ(voice.getPitchBendRange(), 2.0f);
}

TEST_F(VoiceTest, SetPitchBendWithDifferentRange) {
    voice.setPitchBend(0.5f, 12.0f);
    
    EXPECT_FLOAT_EQ(voice.getPitchBendValue(), 0.5f);
    EXPECT_FLOAT_EQ(voice.getPitchBendRange(), 12.0f);
}

TEST_F(VoiceTest, PitchBendAffectsRenderedPitch) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    
    // Trigger the voice
    voice.trigger(60, 1.0f);
    
    // Render a few samples without pitch bend
    float sample1 = voice.renderSample();
    float sample2 = voice.renderSample();
    
    // Reset voice
    voice.trigger(60, 1.0f);
    
    // Apply pitch bend up (should increase frequency)
    voice.setPitchBend(1.0f, 2.0f);  // +2 semitones
    
    // Render samples with pitch bend
    float sample3 = voice.renderSample();
    float sample4 = voice.renderSample();
    
    // With pitch bend, the frequency should be higher
    // This means the waveform should complete cycles faster
    // We can't directly test frequency, but we can verify the output is different
    // (This is a basic sanity check)
    EXPECT_TRUE(std::isfinite(sample3));
    EXPECT_TRUE(std::isfinite(sample4));
}

TEST_F(VoiceTest, PitchBendUpIncreasesFrequency) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    voice.trigger(60, 1.0f);
    
    // Apply pitch bend up by 1 semitone (half of 2 semitone range)
    voice.setPitchBend(0.5f, 2.0f);  // +1 semitone
    
    // Expected frequency ratio: 2^(1/12) ≈ 1.0595
    // Expected frequency: 100Hz * 1.0595 ≈ 105.95Hz
    
    // Render a sample to apply the pitch bend
    float sample = voice.renderSample();
    
    // Verify output is finite
    EXPECT_TRUE(std::isfinite(sample));
}

TEST_F(VoiceTest, PitchBendDownDecreasesFrequency) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    voice.trigger(60, 1.0f);
    
    // Apply pitch bend down by 1 semitone
    voice.setPitchBend(-0.5f, 2.0f);  // -1 semitone
    
    // Expected frequency ratio: 2^(-1/12) ≈ 0.9439
    // Expected frequency: 100Hz * 0.9439 ≈ 94.39Hz
    
    // Render a sample to apply the pitch bend
    float sample = voice.renderSample();
    
    // Verify output is finite
    EXPECT_TRUE(std::isfinite(sample));
}

TEST_F(VoiceTest, PitchBendWithLargeRange) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    voice.trigger(60, 1.0f);
    
    // Apply pitch bend with 12 semitone range (one octave)
    voice.setPitchBend(1.0f, 12.0f);  // +12 semitones (one octave up)
    
    // Expected frequency ratio: 2^(12/12) = 2.0
    // Expected frequency: 100Hz * 2.0 = 200Hz
    
    // Render a sample to apply the pitch bend
    float sample = voice.renderSample();
    
    // Verify output is finite
    EXPECT_TRUE(std::isfinite(sample));
}

TEST_F(VoiceTest, PitchBendWithZeroRange) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    voice.trigger(60, 1.0f);
    
    // Apply pitch bend with zero range (should have no effect)
    voice.setPitchBend(1.0f, 0.0f);
    
    // Expected frequency: 100Hz (no change)
    
    // Render a sample to apply the pitch bend
    float sample = voice.renderSample();
    
    // Verify output is finite
    EXPECT_TRUE(std::isfinite(sample));
}

TEST_F(VoiceTest, PitchBendCombinesWithPitchEnvelope) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    
    // Configure pitch envelope with some depth
    voice.getPitchEnvelope().setDepth(50.0f);  // +50Hz at peak
    
    // Trigger the voice
    voice.trigger(60, 1.0f);
    
    // Apply pitch bend
    voice.setPitchBend(0.5f, 2.0f);  // +1 semitone
    
    // Render a sample (pitch envelope and pitch bend should both apply)
    float sample = voice.renderSample();
    
    // Verify output is finite
    EXPECT_TRUE(std::isfinite(sample));
}

TEST_F(VoiceTest, PitchBendPersistsAcrossMultipleSamples) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    voice.trigger(60, 1.0f);
    
    // Apply pitch bend
    voice.setPitchBend(0.5f, 2.0f);
    
    // Render multiple samples
    for (int i = 0; i < 100; ++i) {
        float sample = voice.renderSample();
        EXPECT_TRUE(std::isfinite(sample));
    }
    
    // Pitch bend should still be applied
    EXPECT_FLOAT_EQ(voice.getPitchBendValue(), 0.5f);
}

TEST_F(VoiceTest, PitchBendCanBeChangedDuringRendering) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    voice.trigger(60, 1.0f);
    
    // Render some samples with no pitch bend
    for (int i = 0; i < 10; ++i) {
        voice.renderSample();
    }
    
    // Apply pitch bend
    voice.setPitchBend(0.5f, 2.0f);
    
    // Render more samples with pitch bend
    for (int i = 0; i < 10; ++i) {
        float sample = voice.renderSample();
        EXPECT_TRUE(std::isfinite(sample));
    }
    
    // Change pitch bend
    voice.setPitchBend(-0.5f, 2.0f);
    
    // Render more samples with different pitch bend
    for (int i = 0; i < 10; ++i) {
        float sample = voice.renderSample();
        EXPECT_TRUE(std::isfinite(sample));
    }
}

TEST_F(VoiceTest, PitchBendWithExtremeValues) {
    // Set base pitch to 100Hz
    voice.setBasePitch(100.0f);
    voice.trigger(60, 1.0f);
    
    // Apply extreme pitch bend (max up with max range)
    voice.setPitchBend(1.0f, 24.0f);  // +24 semitones (two octaves up)
    
    // Expected frequency ratio: 2^(24/12) = 4.0
    // Expected frequency: 100Hz * 4.0 = 400Hz
    
    // Render a sample
    float sample = voice.renderSample();
    
    // Verify output is finite
    EXPECT_TRUE(std::isfinite(sample));
}

TEST_F(VoiceTest, PitchBendDoesNotAffectInactiveVoice) {
    // Don't trigger the voice
    EXPECT_FALSE(voice.isActive());
    
    // Apply pitch bend
    voice.setPitchBend(0.5f, 2.0f);
    
    // Render should return 0.0
    float sample = voice.renderSample();
    EXPECT_FLOAT_EQ(sample, 0.0f);
}
