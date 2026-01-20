#include <gtest/gtest.h>
#include "audio_engine/generators/NoiseGenerator.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace KickDrum;

namespace {
// Helper function to check if a value is approximately equal
bool approxEqual(float a, float b, float epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}
}

// Test fixture for NoiseGenerator tests
class NoiseGeneratorTest : public ::testing::Test {
protected:
    NoiseGenerator generator;
};

// Test: Constructor initializes with default seed
TEST_F(NoiseGeneratorTest, ConstructorInitialization) {
    NoiseGenerator newGenerator;
    EXPECT_NE(newGenerator.getSeed(), 0u);
}

// Test: Constructor with custom seed
TEST_F(NoiseGeneratorTest, ConstructorWithCustomSeed) {
    uint64_t customSeed = 0xDEADBEEFCAFEBABEULL;
    NoiseGenerator newGenerator(customSeed);
    EXPECT_EQ(newGenerator.getSeed(), customSeed);
}

// Test: Constructor with zero seed uses default
TEST_F(NoiseGeneratorTest, ConstructorWithZeroSeedUsesDefault) {
    NoiseGenerator newGenerator(0);
    EXPECT_NE(newGenerator.getSeed(), 0u);
}

// Test: Set and get seed
TEST_F(NoiseGeneratorTest, SetAndGetSeed) {
    uint64_t testSeed = 0x123456789ABCDEF0ULL;
    generator.setSeed(testSeed);
    EXPECT_EQ(generator.getSeed(), testSeed);
}

// Test: Set zero seed uses default
TEST_F(NoiseGeneratorTest, SetZeroSeedUsesDefault) {
    generator.setSeed(0);
    EXPECT_NE(generator.getSeed(), 0u);
}

// Test: Generate produces values in valid range
TEST_F(NoiseGeneratorTest, GenerateProducesValidRange) {
    // Generate 10000 samples and check they're all in [-1.0, 1.0]
    for (int i = 0; i < 10000; ++i) {
        float sample = generator.generate();
        EXPECT_GE(sample, -1.0f) << "Sample " << i << " below -1.0";
        EXPECT_LE(sample, 1.0f) << "Sample " << i << " above 1.0";
        EXPECT_FALSE(std::isnan(sample)) << "Sample " << i << " is NaN";
        EXPECT_FALSE(std::isinf(sample)) << "Sample " << i << " is infinite";
    }
}

// Test: Reset produces same sequence
TEST_F(NoiseGeneratorTest, ResetProducesSameSequence) {
    uint64_t testSeed = 0xABCDEF0123456789ULL;
    generator.setSeed(testSeed);
    
    // Generate first sequence
    std::vector<float> sequence1;
    for (int i = 0; i < 100; ++i) {
        sequence1.push_back(generator.generate());
    }
    
    // Reset and generate second sequence
    generator.reset();
    std::vector<float> sequence2;
    for (int i = 0; i < 100; ++i) {
        sequence2.push_back(generator.generate());
    }
    
    // Sequences should be identical
    ASSERT_EQ(sequence1.size(), sequence2.size());
    for (size_t i = 0; i < sequence1.size(); ++i) {
        EXPECT_EQ(sequence1[i], sequence2[i]) << "Mismatch at index " << i;
    }
}

// Test: Different seeds produce different sequences
TEST_F(NoiseGeneratorTest, DifferentSeedsProduceDifferentSequences) {
    NoiseGenerator gen1(12345);
    NoiseGenerator gen2(67890);
    
    // Generate sequences
    std::vector<float> sequence1;
    std::vector<float> sequence2;
    for (int i = 0; i < 100; ++i) {
        sequence1.push_back(gen1.generate());
        sequence2.push_back(gen2.generate());
    }
    
    // Sequences should be different (at least some samples)
    int differences = 0;
    for (size_t i = 0; i < sequence1.size(); ++i) {
        if (sequence1[i] != sequence2[i]) {
            differences++;
        }
    }
    
    // Expect most samples to be different (allow for tiny chance of collision)
    EXPECT_GT(differences, 90) << "Sequences too similar";
}

// Test: Same seed produces same sequence
TEST_F(NoiseGeneratorTest, SameSeedProducesSameSequence) {
    uint64_t testSeed = 0x1122334455667788ULL;
    NoiseGenerator gen1(testSeed);
    NoiseGenerator gen2(testSeed);
    
    // Generate sequences
    for (int i = 0; i < 100; ++i) {
        float sample1 = gen1.generate();
        float sample2 = gen2.generate();
        EXPECT_EQ(sample1, sample2) << "Mismatch at sample " << i;
    }
}

// Test: Noise distribution uniformity (basic check)
TEST_F(NoiseGeneratorTest, NoiseDistributionUniformity) {
    const int numSamples = 100000;
    const int numBins = 20;
    std::vector<int> bins(numBins, 0);
    
    // Generate samples and count distribution
    for (int i = 0; i < numSamples; ++i) {
        float sample = generator.generate();
        
        // Map [-1.0, 1.0] to bin index [0, numBins-1]
        int binIndex = static_cast<int>((sample + 1.0f) * 0.5f * numBins);
        
        // Clamp to valid range (handle edge case of exactly 1.0)
        binIndex = std::max(0, std::min(numBins - 1, binIndex));
        
        bins[binIndex]++;
    }
    
    // Expected count per bin (uniform distribution)
    int expectedCount = numSamples / numBins;
    
    // Check that each bin is reasonably close to expected count
    // Allow 20% deviation (this is a statistical test, some variation is expected)
    float tolerance = expectedCount * 0.2f;
    
    for (int i = 0; i < numBins; ++i) {
        EXPECT_NEAR(bins[i], expectedCount, tolerance) 
            << "Bin " << i << " count " << bins[i] 
            << " deviates too much from expected " << expectedCount;
    }
}

// Test: Mean should be close to zero
TEST_F(NoiseGeneratorTest, MeanCloseToZero) {
    const int numSamples = 100000;
    double sum = 0.0;
    
    for (int i = 0; i < numSamples; ++i) {
        sum += generator.generate();
    }
    
    double mean = sum / numSamples;
    
    // Mean should be close to 0 (within 0.01 for large sample size)
    EXPECT_NEAR(mean, 0.0, 0.01) << "Mean is " << mean;
}

// Test: Standard deviation should be reasonable for uniform distribution
TEST_F(NoiseGeneratorTest, StandardDeviationReasonable) {
    const int numSamples = 100000;
    std::vector<float> samples;
    samples.reserve(numSamples);
    
    // Generate samples
    for (int i = 0; i < numSamples; ++i) {
        samples.push_back(generator.generate());
    }
    
    // Calculate mean
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    double mean = sum / numSamples;
    
    // Calculate standard deviation
    double sqSum = 0.0;
    for (float sample : samples) {
        double diff = sample - mean;
        sqSum += diff * diff;
    }
    double stdDev = std::sqrt(sqSum / numSamples);
    
    // For uniform distribution on [-1, 1], theoretical std dev is sqrt(1/3) ≈ 0.577
    // Allow some tolerance
    EXPECT_NEAR(stdDev, 0.577, 0.02) << "Standard deviation is " << stdDev;
}

// Test: No consecutive identical samples (extremely unlikely)
TEST_F(NoiseGeneratorTest, NoConsecutiveIdenticalSamples) {
    int consecutiveIdentical = 0;
    float prevSample = generator.generate();
    
    for (int i = 0; i < 10000; ++i) {
        float sample = generator.generate();
        if (sample == prevSample) {
            consecutiveIdentical++;
        }
        prevSample = sample;
    }
    
    // With 64-bit precision, consecutive identical samples should be extremely rare
    // Allow a few due to floating point conversion, but not many
    EXPECT_LT(consecutiveIdentical, 5) << "Too many consecutive identical samples";
}

// Test: Changing seed mid-generation
TEST_F(NoiseGeneratorTest, ChangingSeedMidGeneration) {
    uint64_t seed1 = 111111;
    uint64_t seed2 = 222222;
    
    generator.setSeed(seed1);
    
    // Generate some samples
    std::vector<float> sequence1;
    for (int i = 0; i < 50; ++i) {
        sequence1.push_back(generator.generate());
    }
    
    // Change seed
    generator.setSeed(seed2);
    
    // Generate more samples
    std::vector<float> sequence2;
    for (int i = 0; i < 50; ++i) {
        sequence2.push_back(generator.generate());
    }
    
    // Create reference generators
    NoiseGenerator ref1(seed1);
    NoiseGenerator ref2(seed2);
    
    // Verify first sequence matches seed1
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(sequence1[i], ref1.generate()) << "Mismatch in sequence1 at " << i;
    }
    
    // Verify second sequence matches seed2
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(sequence2[i], ref2.generate()) << "Mismatch in sequence2 at " << i;
    }
}

// Test: Multiple resets maintain consistency
TEST_F(NoiseGeneratorTest, MultipleResetsMaintainConsistency) {
    uint64_t testSeed = 0xFEDCBA9876543210ULL;
    generator.setSeed(testSeed);
    
    // Reset and get first sample multiple times
    std::vector<float> firstSamples;
    for (int i = 0; i < 10; ++i) {
        generator.reset();
        firstSamples.push_back(generator.generate());
    }
    
    // All first samples should be identical
    for (size_t i = 1; i < firstSamples.size(); ++i) {
        EXPECT_EQ(firstSamples[0], firstSamples[i]) 
            << "First sample after reset " << i << " differs";
    }
}

// Test: Long sequence generation (stress test)
TEST_F(NoiseGeneratorTest, LongSequenceGeneration) {
    const int numSamples = 1000000; // 1 million samples
    
    int validSamples = 0;
    for (int i = 0; i < numSamples; ++i) {
        float sample = generator.generate();
        if (sample >= -1.0f && sample <= 1.0f && 
            !std::isnan(sample) && !std::isinf(sample)) {
            validSamples++;
        }
    }
    
    // All samples should be valid
    EXPECT_EQ(validSamples, numSamples);
}

// Test: Edge case - maximum seed value
TEST_F(NoiseGeneratorTest, MaximumSeedValue) {
    uint64_t maxSeed = UINT64_MAX;
    generator.setSeed(maxSeed);
    
    // Should still generate valid samples
    for (int i = 0; i < 100; ++i) {
        float sample = generator.generate();
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
        EXPECT_FALSE(std::isnan(sample));
    }
}

// Test: Edge case - seed value of 1
TEST_F(NoiseGeneratorTest, MinimumNonZeroSeedValue) {
    generator.setSeed(1);
    EXPECT_EQ(generator.getSeed(), 1u);
    
    // Should still generate valid samples
    for (int i = 0; i < 100; ++i) {
        float sample = generator.generate();
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
        EXPECT_FALSE(std::isnan(sample));
    }
}

// Test: Reproducibility across multiple instances
TEST_F(NoiseGeneratorTest, ReproducibilityAcrossInstances) {
    uint64_t testSeed = 0x0F0F0F0F0F0F0F0FULL;
    
    // Create multiple generators with same seed
    NoiseGenerator gen1(testSeed);
    NoiseGenerator gen2(testSeed);
    NoiseGenerator gen3(testSeed);
    
    // All should produce identical sequences
    for (int i = 0; i < 100; ++i) {
        float sample1 = gen1.generate();
        float sample2 = gen2.generate();
        float sample3 = gen3.generate();
        
        EXPECT_EQ(sample1, sample2) << "gen1 and gen2 differ at sample " << i;
        EXPECT_EQ(sample2, sample3) << "gen2 and gen3 differ at sample " << i;
    }
}
