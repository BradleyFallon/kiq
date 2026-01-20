#include <gtest/gtest.h>
#include "audio_engine/generators/SineDriver.h"
#include <cmath>
#include <vector>

using namespace KickDrum;

namespace {
// Helper function to check if a value is approximately equal
bool approxEqual(float a, float b, float epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}
}

// Test fixture for SineDriver tests
class SineDriverTest : public ::testing::Test {
protected:
    SineDriver driver;
    const float sampleRate = 48000.0f;
    const float testFrequency = 440.0f; // A4
};

// Test: Constructor initializes to safe defaults
TEST_F(SineDriverTest, ConstructorInitialization) {
    SineDriver newDriver;
    EXPECT_FALSE(newDriver.isInitialized());
    EXPECT_EQ(newDriver.getFrequency(), 0.0f);
}

// Test: Initialize sets sample rate correctly
TEST_F(SineDriverTest, InitializeSetsampleRate) {
    driver.initialize(sampleRate);
    EXPECT_TRUE(driver.isInitialized());
}

// Test: Initialize with invalid sample rate
TEST_F(SineDriverTest, InitializeWithInvalidSampleRate) {
    driver.initialize(0.0f);
    EXPECT_FALSE(driver.isInitialized());
    
    driver.initialize(-1000.0f);
    EXPECT_FALSE(driver.isInitialized());
}

// Test: Set and get frequency
TEST_F(SineDriverTest, SetAndGetFrequency) {
    driver.initialize(sampleRate);
    driver.setFrequency(testFrequency);
    EXPECT_EQ(driver.getFrequency(), testFrequency);
}

// Test: Set negative frequency clamps to zero
TEST_F(SineDriverTest, SetNegativeFrequencyClampsToZero) {
    driver.initialize(sampleRate);
    driver.setFrequency(-100.0f);
    EXPECT_EQ(driver.getFrequency(), 0.0f);
}

// Test: Generate returns silence when not initialized
TEST_F(SineDriverTest, GenerateReturnsZeroWhenNotInitialized) {
    driver.setFrequency(testFrequency);
    float sample = driver.generate();
    EXPECT_EQ(sample, 0.0f);
}

// Test: Generate produces values in valid range
TEST_F(SineDriverTest, GenerateProducesValidRange) {
    driver.initialize(sampleRate);
    driver.setFrequency(testFrequency);
    
    // Generate 1000 samples and check they're all in [-1.0, 1.0]
    for (int i = 0; i < 1000; ++i) {
        float sample = driver.generate();
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

// Test: Reset sets phase to zero
TEST_F(SineDriverTest, ResetSetsPhaseToZero) {
    driver.initialize(sampleRate);
    driver.setFrequency(testFrequency);
    
    // Generate some samples to advance phase
    for (int i = 0; i < 100; ++i) {
        driver.generate();
    }
    
    // Reset and check that next sample is at phase 0
    driver.reset();
    float sample = driver.generate();
    
    // At phase 0, sin(0) = 0
    EXPECT_TRUE(approxEqual(sample, 0.0f, 0.001f));
}

// Test: Frequency accuracy - verify correct number of cycles
TEST_F(SineDriverTest, FrequencyAccuracy) {
    driver.initialize(sampleRate);
    float testFreq = 100.0f; // 100 Hz
    driver.setFrequency(testFreq);
    driver.reset();
    
    // Generate one second of audio
    int numSamples = static_cast<int>(sampleRate);
    std::vector<float> samples(numSamples);
    
    for (int i = 0; i < numSamples; ++i) {
        samples[i] = driver.generate();
    }
    
    // Count zero crossings (positive to negative)
    int zeroCrossings = 0;
    for (int i = 1; i < numSamples; ++i) {
        if (samples[i-1] >= 0.0f && samples[i] < 0.0f) {
            zeroCrossings++;
        }
    }
    
    // Each cycle has 2 zero crossings (positive-to-negative and negative-to-positive)
    // We're only counting positive-to-negative, so we expect ~100 crossings for 100 Hz
    EXPECT_NEAR(zeroCrossings, testFreq, 2); // Allow ±2 crossings tolerance
}

// Test: Phase continuity during frequency changes
TEST_F(SineDriverTest, PhaseContinuityDuringFrequencyChange) {
    driver.initialize(sampleRate);
    driver.setFrequency(440.0f);
    driver.reset();
    
    // Generate some samples
    std::vector<float> samples;
    for (int i = 0; i < 100; ++i) {
        samples.push_back(driver.generate());
    }
    
    // Change frequency
    driver.setFrequency(880.0f);
    
    // Generate more samples
    for (int i = 0; i < 100; ++i) {
        samples.push_back(driver.generate());
    }
    
    // Check for discontinuities (large jumps between consecutive samples)
    for (size_t i = 1; i < samples.size(); ++i) {
        float diff = std::abs(samples[i] - samples[i-1]);
        // Maximum difference between consecutive samples should be reasonable
        // For 880 Hz at 48kHz, max change per sample is approximately 2*PI*880/48000 ≈ 0.115
        EXPECT_LT(diff, 0.2f) << "Discontinuity detected at sample " << i;
    }
}

// Test: Sample-accurate frequency changes
TEST_F(SineDriverTest, SampleAccurateFrequencyChanges) {
    driver.initialize(sampleRate);
    driver.setFrequency(100.0f);
    driver.reset();
    
    // Generate samples and change frequency multiple times
    for (int i = 0; i < 10; ++i) {
        driver.generate();
    }
    
    driver.setFrequency(200.0f);
    float sample1 = driver.generate();
    
    driver.setFrequency(300.0f);
    float sample2 = driver.generate();
    
    // Samples should be valid (not NaN or infinity)
    EXPECT_FALSE(std::isnan(sample1));
    EXPECT_FALSE(std::isinf(sample1));
    EXPECT_FALSE(std::isnan(sample2));
    EXPECT_FALSE(std::isinf(sample2));
}

// Test: Zero frequency produces DC offset (zero)
TEST_F(SineDriverTest, ZeroFrequencyProducesDC) {
    driver.initialize(sampleRate);
    driver.setFrequency(0.0f);
    driver.reset();
    
    // Generate samples - should all be zero (or very close)
    for (int i = 0; i < 100; ++i) {
        float sample = driver.generate();
        EXPECT_TRUE(approxEqual(sample, 0.0f, 0.001f));
    }
}

// Test: Very low frequency (20 Hz - kick drum range)
TEST_F(SineDriverTest, VeryLowFrequency) {
    driver.initialize(sampleRate);
    driver.setFrequency(20.0f); // Low end of kick drum range
    driver.reset();
    
    // Generate samples and verify they're valid
    for (int i = 0; i < 1000; ++i) {
        float sample = driver.generate();
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
        EXPECT_FALSE(std::isnan(sample));
    }
}

// Test: High frequency (200 Hz - upper kick drum range)
TEST_F(SineDriverTest, HighFrequency) {
    driver.initialize(sampleRate);
    driver.setFrequency(200.0f); // High end of kick drum range
    driver.reset();
    
    // Generate samples and verify they're valid
    for (int i = 0; i < 1000; ++i) {
        float sample = driver.generate();
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
        EXPECT_FALSE(std::isnan(sample));
    }
}

// Test: Multiple resets maintain consistency
TEST_F(SineDriverTest, MultipleResetsMaintainConsistency) {
    driver.initialize(sampleRate);
    driver.setFrequency(testFrequency);
    
    // Reset and get first sample
    driver.reset();
    float sample1 = driver.generate();
    
    // Generate more samples
    for (int i = 0; i < 100; ++i) {
        driver.generate();
    }
    
    // Reset again and get first sample
    driver.reset();
    float sample2 = driver.generate();
    
    // Both samples should be the same (phase 0)
    EXPECT_TRUE(approxEqual(sample1, sample2, 0.0001f));
}

// Test: Sine wave amplitude peaks at approximately ±1.0
TEST_F(SineDriverTest, AmplitudePeaks) {
    driver.initialize(sampleRate);
    driver.setFrequency(100.0f);
    driver.reset();
    
    // Generate one complete cycle and find min/max
    int samplesPerCycle = static_cast<int>(sampleRate / 100.0f);
    float maxSample = -2.0f;
    float minSample = 2.0f;
    
    for (int i = 0; i < samplesPerCycle; ++i) {
        float sample = driver.generate();
        maxSample = std::max(maxSample, sample);
        minSample = std::min(minSample, sample);
    }
    
    // Check that peaks are close to ±1.0
    EXPECT_TRUE(approxEqual(maxSample, 1.0f, 0.01f));
    EXPECT_TRUE(approxEqual(minSample, -1.0f, 0.01f));
}

// Test: Different sample rates
TEST_F(SineDriverTest, DifferentSampleRates) {
    std::vector<float> sampleRates = {44100.0f, 48000.0f, 88200.0f, 96000.0f, 192000.0f};
    
    for (float sr : sampleRates) {
        SineDriver testDriver;
        testDriver.initialize(sr);
        testDriver.setFrequency(440.0f);
        testDriver.reset();
        
        // Generate samples and verify they're valid
        for (int i = 0; i < 100; ++i) {
            float sample = testDriver.generate();
            EXPECT_GE(sample, -1.0f) << "Failed at sample rate " << sr;
            EXPECT_LE(sample, 1.0f) << "Failed at sample rate " << sr;
            EXPECT_FALSE(std::isnan(sample)) << "Failed at sample rate " << sr;
        }
    }
}
