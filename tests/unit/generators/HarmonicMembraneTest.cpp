#include <gtest/gtest.h>
#include "audio_engine/generators/HarmonicMembrane.h"
#include <cmath>

using namespace KickDrum;

// Test fixture for HarmonicMembrane tests
class HarmonicMembraneTest : public ::testing::Test {
protected:
    void SetUp() override {
        membrane = std::make_unique<HarmonicMembrane>();
    }

    std::unique_ptr<HarmonicMembrane> membrane;
    
    // Helper constants
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr float BASE_FREQUENCY = 100.0f;
    static constexpr float EPSILON = 1e-5f;
};

// Test initialization
TEST_F(HarmonicMembraneTest, InitializationState) {
    EXPECT_FALSE(membrane->isInitialized());
    
    membrane->initialize(SAMPLE_RATE);
    
    EXPECT_TRUE(membrane->isInitialized());
}

TEST_F(HarmonicMembraneTest, InitializationWithInvalidSampleRate) {
    membrane->initialize(0.0f);
    EXPECT_FALSE(membrane->isInitialized());
    
    membrane->initialize(-1000.0f);
    EXPECT_FALSE(membrane->isInitialized());
}

// Test base frequency control
TEST_F(HarmonicMembraneTest, SetBaseFrequency) {
    membrane->initialize(SAMPLE_RATE);
    
    membrane->setBaseFrequency(BASE_FREQUENCY);
    EXPECT_FLOAT_EQ(membrane->getBaseFrequency(), BASE_FREQUENCY);
    
    membrane->setBaseFrequency(50.0f);
    EXPECT_FLOAT_EQ(membrane->getBaseFrequency(), 50.0f);
}

TEST_F(HarmonicMembraneTest, SetBaseFrequencyNegativeClamped) {
    membrane->initialize(SAMPLE_RATE);
    
    membrane->setBaseFrequency(-100.0f);
    EXPECT_FLOAT_EQ(membrane->getBaseFrequency(), 0.0f);
}

// Test ratio control
TEST_F(HarmonicMembraneTest, SetRatioInRange) {
    membrane->initialize(SAMPLE_RATE);
    
    membrane->setRatio(1.0f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 1.0f);
    
    membrane->setRatio(2.0f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 2.0f);
    
    membrane->setRatio(0.5f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 0.5f);
    
    membrane->setRatio(8.0f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 8.0f);
}

TEST_F(HarmonicMembraneTest, SetRatioClampedToMinimum) {
    membrane->initialize(SAMPLE_RATE);
    
    membrane->setRatio(0.1f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 0.5f);
    
    membrane->setRatio(0.0f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 0.5f);
    
    membrane->setRatio(-1.0f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 0.5f);
}

TEST_F(HarmonicMembraneTest, SetRatioClampedToMaximum) {
    membrane->initialize(SAMPLE_RATE);
    
    membrane->setRatio(10.0f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 8.0f);
    
    membrane->setRatio(100.0f);
    EXPECT_FLOAT_EQ(membrane->getRatio(), 8.0f);
}

// Test actual frequency calculation
TEST_F(HarmonicMembraneTest, GetFrequencyCalculation) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(BASE_FREQUENCY);
    
    membrane->setRatio(1.0f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 100.0f);
    
    membrane->setRatio(2.0f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 200.0f);
    
    membrane->setRatio(0.5f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 50.0f);
    
    membrane->setRatio(4.0f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 400.0f);
}

TEST_F(HarmonicMembraneTest, GetFrequencyWithDifferentBaseFrequencies) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setRatio(2.0f);
    
    membrane->setBaseFrequency(50.0f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 100.0f);
    
    membrane->setBaseFrequency(200.0f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 400.0f);
}

// Test phase reset
TEST_F(HarmonicMembraneTest, PhaseReset) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(BASE_FREQUENCY);
    membrane->setRatio(1.0f);
    
    // Generate some samples to advance phase
    for (int i = 0; i < 100; ++i) {
        membrane->generate();
    }
    
    // Reset phase
    membrane->reset();
    
    // First sample after reset should be close to 0 (sine of 0)
    float firstSample = membrane->generate();
    EXPECT_NEAR(firstSample, 0.0f, 0.01f);
}

// Test sample generation
TEST_F(HarmonicMembraneTest, GenerateReturnsValidRange) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(BASE_FREQUENCY);
    membrane->setRatio(1.0f);
    
    // Generate samples and verify they're in valid range [-1.0, 1.0]
    for (int i = 0; i < 1000; ++i) {
        float sample = membrane->generate();
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

TEST_F(HarmonicMembraneTest, GenerateWithoutInitializationReturnsSilence) {
    float sample = membrane->generate();
    EXPECT_FLOAT_EQ(sample, 0.0f);
}

// Test frequency accuracy
TEST_F(HarmonicMembraneTest, FrequencyAccuracy) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(BASE_FREQUENCY);
    membrane->setRatio(2.0f); // 200 Hz
    membrane->reset();
    
    // Generate one full cycle worth of samples
    // At 200 Hz and 48000 Hz sample rate, one cycle = 48000/200 = 240 samples
    int samplesPerCycle = static_cast<int>(SAMPLE_RATE / 200.0f);
    
    float firstSample = membrane->generate();
    
    // Generate remaining samples in the cycle
    for (int i = 1; i < samplesPerCycle; ++i) {
        membrane->generate();
    }
    
    // After one complete cycle, the next sample should be very close to the first
    float nextCycleSample = membrane->generate();
    EXPECT_NEAR(firstSample, nextCycleSample, 0.01f);
}

// Test phase continuity during parameter changes
TEST_F(HarmonicMembraneTest, PhaseContinuityOnRatioChange) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(BASE_FREQUENCY);
    membrane->setRatio(1.0f);
    
    // Generate some samples
    for (int i = 0; i < 50; ++i) {
        membrane->generate();
    }
    
    float sampleBeforeChange = membrane->generate();
    
    // Change ratio
    membrane->setRatio(2.0f);
    
    float sampleAfterChange = membrane->generate();
    
    // Samples should be continuous (no sudden jump)
    // The difference should be reasonable for adjacent samples
    float difference = std::abs(sampleAfterChange - sampleBeforeChange);
    EXPECT_LT(difference, 0.5f); // Reasonable threshold for continuity
}

TEST_F(HarmonicMembraneTest, PhaseContinuityOnBaseFrequencyChange) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(BASE_FREQUENCY);
    membrane->setRatio(1.0f);
    
    // Generate some samples
    for (int i = 0; i < 50; ++i) {
        membrane->generate();
    }
    
    float sampleBeforeChange = membrane->generate();
    
    // Change base frequency
    membrane->setBaseFrequency(150.0f);
    
    float sampleAfterChange = membrane->generate();
    
    // Samples should be continuous (no sudden jump)
    float difference = std::abs(sampleAfterChange - sampleBeforeChange);
    EXPECT_LT(difference, 0.5f); // Reasonable threshold for continuity
}

// Test edge cases
TEST_F(HarmonicMembraneTest, MinimumRatioFrequency) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(BASE_FREQUENCY);
    membrane->setRatio(0.5f); // Minimum ratio
    
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 50.0f);
    
    // Should generate valid samples
    for (int i = 0; i < 100; ++i) {
        float sample = membrane->generate();
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

TEST_F(HarmonicMembraneTest, MaximumRatioFrequency) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(BASE_FREQUENCY);
    membrane->setRatio(8.0f); // Maximum ratio
    
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 800.0f);
    
    // Should generate valid samples
    for (int i = 0; i < 100; ++i) {
        float sample = membrane->generate();
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

TEST_F(HarmonicMembraneTest, ZeroBaseFrequency) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setBaseFrequency(0.0f);
    membrane->setRatio(2.0f);
    
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 0.0f);
    
    // Should generate silence (or near-silence)
    for (int i = 0; i < 100; ++i) {
        float sample = membrane->generate();
        EXPECT_NEAR(sample, 0.0f, EPSILON);
    }
}

// Test that HarmonicMembrane tracks base frequency changes
TEST_F(HarmonicMembraneTest, TracksBaseFrequencyFromSineDriver) {
    membrane->initialize(SAMPLE_RATE);
    membrane->setRatio(2.0f);
    
    // Simulate Sine Driver frequency changes
    membrane->setBaseFrequency(50.0f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 100.0f);
    
    membrane->setBaseFrequency(100.0f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 200.0f);
    
    membrane->setBaseFrequency(75.0f);
    EXPECT_FLOAT_EQ(membrane->getFrequency(), 150.0f);
}
