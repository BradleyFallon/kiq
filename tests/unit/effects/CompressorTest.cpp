#include <gtest/gtest.h>
#include "audio_engine/effects/Compressor.h"
#include <cmath>
#include <vector>

using namespace KickDrum;

class CompressorTest : public ::testing::Test {
protected:
    void SetUp() override {
        compressor = std::make_unique<Compressor>();
        sampleRate = 48000.0f;
        compressor->initialize(sampleRate);
    }

    std::unique_ptr<Compressor> compressor;
    float sampleRate;
    
    // Helper function to generate a sine wave
    std::vector<float> generateSineWave(float frequency, float amplitude, float durationSeconds) {
        int numSamples = static_cast<int>(durationSeconds * sampleRate);
        std::vector<float> samples(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            float phase = 2.0f * M_PI * frequency * i / sampleRate;
            samples[i] = amplitude * std::sin(phase);
        }
        
        return samples;
    }
    
    // Helper function to calculate RMS (root mean square) of a signal
    float calculateRMS(const std::vector<float>& samples) {
        float sum = 0.0f;
        for (float sample : samples) {
            sum += sample * sample;
        }
        return std::sqrt(sum / samples.size());
    }
    
    // Helper function to convert linear to dB
    float linearToDb(float linear) {
        if (linear < 1e-10f) return -96.0f;
        return 20.0f * std::log10(linear);
    }
    
    // Helper function to convert dB to linear
    float dbToLinear(float db) {
        return std::pow(10.0f, db / 20.0f);
    }
};

// Test 1: Initialization
TEST_F(CompressorTest, Initialization) {
    Compressor comp;
    EXPECT_FALSE(comp.isInitialized());
    
    comp.initialize(48000.0f);
    EXPECT_TRUE(comp.isInitialized());
}

// Test 2: Invalid sample rate
TEST_F(CompressorTest, InvalidSampleRate) {
    Compressor comp;
    comp.initialize(0.0f);
    EXPECT_FALSE(comp.isInitialized());
    
    comp.initialize(-48000.0f);
    EXPECT_FALSE(comp.isInitialized());
}

// Test 3: Default parameters
TEST_F(CompressorTest, DefaultParameters) {
    EXPECT_EQ(compressor->getThreshold(), -12.0f);
    EXPECT_EQ(compressor->getRatio(), 4.0f);
    EXPECT_EQ(compressor->getAttack(), 0.005f);
    EXPECT_EQ(compressor->getRelease(), 0.1f);
    EXPECT_EQ(compressor->getMix(), 1.0f);
}

// Test 4: Parameter setters and getters
TEST_F(CompressorTest, ParameterSettersGetters) {
    compressor->setThreshold(-20.0f);
    EXPECT_EQ(compressor->getThreshold(), -20.0f);
    
    compressor->setRatio(8.0f);
    EXPECT_EQ(compressor->getRatio(), 8.0f);
    
    compressor->setAttack(0.01f);
    EXPECT_EQ(compressor->getAttack(), 0.01f);
    
    compressor->setRelease(0.2f);
    EXPECT_EQ(compressor->getRelease(), 0.2f);
    
    compressor->setMix(0.5f);
    EXPECT_EQ(compressor->getMix(), 0.5f);
}

// Test 5: Parameter clamping - threshold
TEST_F(CompressorTest, ThresholdClamping) {
    compressor->setThreshold(-100.0f);
    EXPECT_EQ(compressor->getThreshold(), -60.0f); // Clamped to minimum
    
    compressor->setThreshold(10.0f);
    EXPECT_EQ(compressor->getThreshold(), 0.0f); // Clamped to maximum
}

// Test 6: Parameter clamping - ratio
TEST_F(CompressorTest, RatioClamping) {
    compressor->setRatio(0.5f);
    EXPECT_EQ(compressor->getRatio(), 1.0f); // Clamped to minimum
    
    compressor->setRatio(50.0f);
    EXPECT_EQ(compressor->getRatio(), 20.0f); // Clamped to maximum
}

// Test 7: Parameter clamping - attack
TEST_F(CompressorTest, AttackClamping) {
    compressor->setAttack(0.00001f);
    EXPECT_EQ(compressor->getAttack(), 0.0001f); // Clamped to minimum
    
    compressor->setAttack(1.0f);
    EXPECT_EQ(compressor->getAttack(), 0.1f); // Clamped to maximum
}

// Test 8: Parameter clamping - release
TEST_F(CompressorTest, ReleaseClamping) {
    compressor->setRelease(0.001f);
    EXPECT_EQ(compressor->getRelease(), 0.01f); // Clamped to minimum
    
    compressor->setRelease(10.0f);
    EXPECT_EQ(compressor->getRelease(), 1.0f); // Clamped to maximum
}

// Test 9: Parameter clamping - mix
TEST_F(CompressorTest, MixClamping) {
    compressor->setMix(-0.5f);
    EXPECT_EQ(compressor->getMix(), 0.0f); // Clamped to minimum
    
    compressor->setMix(2.0f);
    EXPECT_EQ(compressor->getMix(), 1.0f); // Clamped to maximum
}

// Test 10: Bypass when not initialized
TEST_F(CompressorTest, BypassWhenNotInitialized) {
    Compressor comp;
    float input = 0.5f;
    float output = comp.process(input);
    EXPECT_EQ(output, input); // Should pass through unchanged
}

// Test 11: No compression below threshold
TEST_F(CompressorTest, NoCompressionBelowThreshold) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    
    // Generate a quiet signal (-30dB, which is below -20dB threshold)
    float amplitude = dbToLinear(-30.0f);
    auto input = generateSineWave(100.0f, amplitude, 0.1f);
    
    // Process the signal
    std::vector<float> output;
    for (float sample : input) {
        output.push_back(compressor->process(sample));
    }
    
    // Calculate RMS of input and output
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // Output should be very close to input (no compression)
    EXPECT_NEAR(outputRMS, inputRMS, 0.01f);
}

// Test 12: Compression above threshold
TEST_F(CompressorTest, CompressionAboveThreshold) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    compressor->setAttack(0.001f);  // Fast attack
    compressor->setRelease(0.05f);   // Fast release
    
    // Generate a loud signal (-10dB, which is 10dB above -20dB threshold)
    float amplitude = dbToLinear(-10.0f);
    auto input = generateSineWave(100.0f, amplitude, 0.1f);
    
    // Process the signal
    std::vector<float> output;
    for (float sample : input) {
        output.push_back(compressor->process(sample));
    }
    
    // Calculate RMS of input and output
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // Output should be quieter than input (compression applied)
    EXPECT_LT(outputRMS, inputRMS);
    
    // With 4:1 ratio and 10dB over threshold:
    // Expected gain reduction = 10dB × (1 - 1/4) = 7.5dB
    // So output should be approximately 7.5dB quieter
    float inputDb = linearToDb(inputRMS);
    float outputDb = linearToDb(outputRMS);
    float actualReduction = inputDb - outputDb;
    
    // Allow some tolerance due to attack/release envelope
    EXPECT_NEAR(actualReduction, 7.5f, 2.0f);
}

// Test 13: Ratio of 1.0 (no compression)
TEST_F(CompressorTest, RatioOneNoCompression) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(1.0f);  // No compression
    compressor->setMix(1.0f);
    
    // Generate a loud signal above threshold
    float amplitude = dbToLinear(-10.0f);
    auto input = generateSineWave(100.0f, amplitude, 0.1f);
    
    // Process the signal
    std::vector<float> output;
    for (float sample : input) {
        output.push_back(compressor->process(sample));
    }
    
    // Calculate RMS
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // With ratio = 1.0, no compression should occur
    EXPECT_NEAR(outputRMS, inputRMS, 0.01f);
}

// Test 14: High ratio (limiting)
TEST_F(CompressorTest, HighRatioLimiting) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(20.0f);  // Heavy limiting
    compressor->setMix(1.0f);
    compressor->setAttack(0.001f);
    compressor->setRelease(0.05f);
    
    // Generate a loud signal
    float amplitude = dbToLinear(-10.0f);
    auto input = generateSineWave(100.0f, amplitude, 0.1f);
    
    // Process the signal
    std::vector<float> output;
    for (float sample : input) {
        output.push_back(compressor->process(sample));
    }
    
    // Calculate RMS
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // With 20:1 ratio and 10dB over threshold:
    // Expected gain reduction = 10dB × (1 - 1/20) = 9.5dB
    float inputDb = linearToDb(inputRMS);
    float outputDb = linearToDb(outputRMS);
    float actualReduction = inputDb - outputDb;
    
    EXPECT_NEAR(actualReduction, 9.5f, 2.0f);
}

// Test 15: Dry/wet mix at 0% (fully dry)
TEST_F(CompressorTest, FullyDryMix) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(0.0f);  // Fully dry
    
    // Generate a loud signal
    float amplitude = dbToLinear(-10.0f);
    auto input = generateSineWave(100.0f, amplitude, 0.1f);
    
    // Process the signal
    std::vector<float> output;
    for (float sample : input) {
        output.push_back(compressor->process(sample));
    }
    
    // Calculate RMS
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // With mix = 0.0, output should equal input
    EXPECT_NEAR(outputRMS, inputRMS, 0.001f);
}

// Test 16: Dry/wet mix at 50%
TEST_F(CompressorTest, HalfMix) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(0.5f);  // 50% mix
    compressor->setAttack(0.001f);
    compressor->setRelease(0.05f);
    
    // Generate a loud signal
    float amplitude = dbToLinear(-10.0f);
    auto input = generateSineWave(100.0f, amplitude, 0.1f);
    
    // Process with full wet
    compressor->setMix(1.0f);
    std::vector<float> fullWet;
    for (float sample : input) {
        fullWet.push_back(compressor->process(sample));
    }
    
    // Reset and process with 50% mix
    compressor->reset();
    compressor->setMix(0.5f);
    std::vector<float> halfMix;
    for (float sample : input) {
        halfMix.push_back(compressor->process(sample));
    }
    
    // Calculate RMS
    float inputRMS = calculateRMS(input);
    float fullWetRMS = calculateRMS(fullWet);
    float halfMixRMS = calculateRMS(halfMix);
    
    // Half mix should be between input and full wet
    EXPECT_GT(halfMixRMS, fullWetRMS);
    EXPECT_LT(halfMixRMS, inputRMS);
}

// Test 17: Reset clears gain reduction
TEST_F(CompressorTest, ResetClearsGainReduction) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    
    // Process a loud signal to build up gain reduction
    float amplitude = dbToLinear(-10.0f);
    for (int i = 0; i < 1000; ++i) {
        compressor->process(amplitude);
    }
    
    // Gain reduction should be non-zero
    EXPECT_GT(compressor->getGainReduction(), 0.0f);
    
    // Reset
    compressor->reset();
    
    // Gain reduction should be zero
    EXPECT_EQ(compressor->getGainReduction(), 0.0f);
}

// Test 18: Attack time affects response speed
TEST_F(CompressorTest, AttackTimeAffectsResponse) {
    // Test with fast attack
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    compressor->setAttack(0.001f);  // 1ms attack
    compressor->setRelease(0.1f);
    
    float amplitude = dbToLinear(-10.0f);
    
    // Process 100 samples
    for (int i = 0; i < 100; ++i) {
        compressor->process(amplitude);
    }
    float fastAttackGainReduction = compressor->getGainReduction();
    
    // Reset and test with slow attack
    compressor->reset();
    compressor->setAttack(0.05f);  // 50ms attack
    
    for (int i = 0; i < 100; ++i) {
        compressor->process(amplitude);
    }
    float slowAttackGainReduction = compressor->getGainReduction();
    
    // Fast attack should have more gain reduction after same number of samples
    EXPECT_GT(fastAttackGainReduction, slowAttackGainReduction);
}

// Test 19: Release time affects response speed
TEST_F(CompressorTest, ReleaseTimeAffectsResponse) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    compressor->setAttack(0.001f);
    
    float loudAmplitude = dbToLinear(-10.0f);
    float quietAmplitude = dbToLinear(-30.0f);
    
    // Build up gain reduction with fast release
    compressor->setRelease(0.01f);  // 10ms release
    for (int i = 0; i < 1000; ++i) {
        compressor->process(loudAmplitude);
    }
    
    // Switch to quiet signal
    for (int i = 0; i < 100; ++i) {
        compressor->process(quietAmplitude);
    }
    float fastReleaseGainReduction = compressor->getGainReduction();
    
    // Reset and test with slow release
    compressor->reset();
    compressor->setRelease(0.5f);  // 500ms release
    
    for (int i = 0; i < 1000; ++i) {
        compressor->process(loudAmplitude);
    }
    
    for (int i = 0; i < 100; ++i) {
        compressor->process(quietAmplitude);
    }
    float slowReleaseGainReduction = compressor->getGainReduction();
    
    // Slow release should have more gain reduction remaining
    EXPECT_GT(slowReleaseGainReduction, fastReleaseGainReduction);
}

// Test 20: Zero input handling
TEST_F(CompressorTest, ZeroInputHandling) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    
    float output = compressor->process(0.0f);
    
    // Should handle zero input without crashing or producing NaN
    EXPECT_FALSE(std::isnan(output));
    EXPECT_FALSE(std::isinf(output));
    EXPECT_EQ(output, 0.0f);
}

// Test 21: Very small input handling
TEST_F(CompressorTest, VerySmallInputHandling) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    
    float input = 1e-12f;  // Very small value
    float output = compressor->process(input);
    
    // Should handle very small input without crashing or producing NaN
    EXPECT_FALSE(std::isnan(output));
    EXPECT_FALSE(std::isinf(output));
}

// Test 22: Negative input handling
TEST_F(CompressorTest, NegativeInputHandling) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    
    float input = -0.5f;
    float output = compressor->process(input);
    
    // Should handle negative input correctly
    EXPECT_FALSE(std::isnan(output));
    EXPECT_FALSE(std::isinf(output));
    EXPECT_LT(output, 0.0f);  // Output should also be negative
}

// Test 23: Gain reduction is always non-negative
TEST_F(CompressorTest, GainReductionNonNegative) {
    compressor->setThreshold(-20.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    
    // Process various signal levels
    std::vector<float> testAmplitudes = {0.0f, 0.1f, 0.5f, 1.0f};
    
    for (float amplitude : testAmplitudes) {
        compressor->reset();
        for (int i = 0; i < 100; ++i) {
            compressor->process(amplitude);
        }
        
        // Gain reduction should always be >= 0
        EXPECT_GE(compressor->getGainReduction(), 0.0f);
    }
}

// Test 24: Threshold at 0dB (no compression for signals <= 0dBFS)
TEST_F(CompressorTest, ThresholdAtZeroDb) {
    compressor->setThreshold(0.0f);
    compressor->setRatio(4.0f);
    compressor->setMix(1.0f);
    
    // Generate a signal at -6dB (below 0dB threshold)
    float amplitude = dbToLinear(-6.0f);
    auto input = generateSineWave(100.0f, amplitude, 0.1f);
    
    std::vector<float> output;
    for (float sample : input) {
        output.push_back(compressor->process(sample));
    }
    
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    // Should have minimal compression
    EXPECT_NEAR(outputRMS, inputRMS, 0.01f);
}
