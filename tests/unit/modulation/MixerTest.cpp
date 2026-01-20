#include <gtest/gtest.h>
#include "audio_engine/modulation/GeneratorMixer.h"
#include <cmath>

using namespace KickDrum;

// Test fixture for GeneratorMixer tests
class GeneratorMixerTest : public ::testing::Test {
protected:
    GeneratorMixer mixer;
};

// Test default constructor initializes all levels to 0.0
TEST_F(GeneratorMixerTest, DefaultConstructorInitializesLevelsToZero) {
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 0.0f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 0.0f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 0.0f);
}

// Test sine level setter and getter
TEST_F(GeneratorMixerTest, SetAndGetSineLevel) {
    mixer.setSineLevel(0.5f);
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 0.5f);
    
    mixer.setSineLevel(1.0f);
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 1.0f);
    
    mixer.setSineLevel(0.0f);
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 0.0f);
}

// Test harmonic level setter and getter
TEST_F(GeneratorMixerTest, SetAndGetHarmonicLevel) {
    mixer.setHarmonicLevel(0.3f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 0.3f);
    
    mixer.setHarmonicLevel(1.0f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 1.0f);
    
    mixer.setHarmonicLevel(0.0f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 0.0f);
}

// Test noise level setter and getter
TEST_F(GeneratorMixerTest, SetAndGetNoiseLevel) {
    mixer.setNoiseLevel(0.7f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 0.7f);
    
    mixer.setNoiseLevel(1.0f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 1.0f);
    
    mixer.setNoiseLevel(0.0f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 0.0f);
}

// Test level clamping - values above 1.0 should be clamped
TEST_F(GeneratorMixerTest, LevelClampingAboveRange) {
    mixer.setSineLevel(1.5f);
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 1.0f);
    
    mixer.setHarmonicLevel(2.0f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 1.0f);
    
    mixer.setNoiseLevel(10.0f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 1.0f);
}

// Test level clamping - values below 0.0 should be clamped
TEST_F(GeneratorMixerTest, LevelClampingBelowRange) {
    mixer.setSineLevel(-0.5f);
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 0.0f);
    
    mixer.setHarmonicLevel(-1.0f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 0.0f);
    
    mixer.setNoiseLevel(-10.0f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 0.0f);
}

// Test mixing with all levels at 0.0 produces silence
TEST_F(GeneratorMixerTest, MixingWithZeroLevelsProducesSilence) {
    float sine = 0.5f;
    float harmonic = 0.3f;
    float noise = 0.7f;
    
    float output = mixer.mix(sine, harmonic, noise);
    EXPECT_FLOAT_EQ(output, 0.0f);
}

// Test mixing with only sine level active
TEST_F(GeneratorMixerTest, MixingWithOnlySineLevel) {
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(0.0f);
    mixer.setNoiseLevel(0.0f);
    
    float sine = 0.5f;
    float harmonic = 0.3f;
    float noise = 0.7f;
    
    float output = mixer.mix(sine, harmonic, noise);
    EXPECT_FLOAT_EQ(output, 0.5f);
}

// Test mixing with only harmonic level active
TEST_F(GeneratorMixerTest, MixingWithOnlyHarmonicLevel) {
    mixer.setSineLevel(0.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(0.0f);
    
    float sine = 0.5f;
    float harmonic = 0.3f;
    float noise = 0.7f;
    
    float output = mixer.mix(sine, harmonic, noise);
    EXPECT_FLOAT_EQ(output, 0.3f);
}

// Test mixing with only noise level active
TEST_F(GeneratorMixerTest, MixingWithOnlyNoiseLevel) {
    mixer.setSineLevel(0.0f);
    mixer.setHarmonicLevel(0.0f);
    mixer.setNoiseLevel(1.0f);
    
    float sine = 0.5f;
    float harmonic = 0.3f;
    float noise = 0.7f;
    
    float output = mixer.mix(sine, harmonic, noise);
    EXPECT_FLOAT_EQ(output, 0.7f);
}

// Test mixing with all levels at 1.0
TEST_F(GeneratorMixerTest, MixingWithAllLevelsAtOne) {
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(1.0f);
    
    float sine = 0.2f;
    float harmonic = 0.3f;
    float noise = 0.1f;
    
    float output = mixer.mix(sine, harmonic, noise);
    float expected = 0.2f + 0.3f + 0.1f;
    EXPECT_FLOAT_EQ(output, expected);
}

// Test mixing with partial levels
TEST_F(GeneratorMixerTest, MixingWithPartialLevels) {
    mixer.setSineLevel(0.5f);
    mixer.setHarmonicLevel(0.3f);
    mixer.setNoiseLevel(0.8f);
    
    float sine = 0.4f;
    float harmonic = 0.6f;
    float noise = 0.2f;
    
    float output = mixer.mix(sine, harmonic, noise);
    float expected = (0.4f * 0.5f) + (0.6f * 0.3f) + (0.2f * 0.8f);
    EXPECT_FLOAT_EQ(output, expected);
}

// Test mixing with negative input samples
TEST_F(GeneratorMixerTest, MixingWithNegativeInputs) {
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(1.0f);
    
    float sine = -0.5f;
    float harmonic = -0.3f;
    float noise = -0.2f;
    
    float output = mixer.mix(sine, harmonic, noise);
    float expected = -0.5f + (-0.3f) + (-0.2f);
    EXPECT_FLOAT_EQ(output, expected);
}

// Test mixing with mixed positive and negative inputs
TEST_F(GeneratorMixerTest, MixingWithMixedSignInputs) {
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(1.0f);
    
    float sine = 0.5f;
    float harmonic = -0.3f;
    float noise = 0.2f;
    
    float output = mixer.mix(sine, harmonic, noise);
    float expected = 0.5f + (-0.3f) + 0.2f;
    EXPECT_FLOAT_EQ(output, expected);
}

// Test mixing with zero input samples
TEST_F(GeneratorMixerTest, MixingWithZeroInputs) {
    mixer.setSineLevel(0.5f);
    mixer.setHarmonicLevel(0.3f);
    mixer.setNoiseLevel(0.8f);
    
    float output = mixer.mix(0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(output, 0.0f);
}

// Test mixing formula correctness with specific values
TEST_F(GeneratorMixerTest, MixingFormulaCorrectness) {
    mixer.setSineLevel(0.6f);
    mixer.setHarmonicLevel(0.4f);
    mixer.setNoiseLevel(0.2f);
    
    float sine = 1.0f;
    float harmonic = 0.5f;
    float noise = -0.5f;
    
    float output = mixer.mix(sine, harmonic, noise);
    // Expected: (1.0 * 0.6) + (0.5 * 0.4) + (-0.5 * 0.2)
    //         = 0.6 + 0.2 + (-0.1)
    //         = 0.7
    EXPECT_FLOAT_EQ(output, 0.7f);
}

// Test that mixing is linear (superposition principle)
TEST_F(GeneratorMixerTest, MixingIsLinear) {
    mixer.setSineLevel(0.5f);
    mixer.setHarmonicLevel(0.3f);
    mixer.setNoiseLevel(0.7f);
    
    // Mix first set of inputs
    float output1 = mixer.mix(0.2f, 0.4f, 0.1f);
    
    // Mix second set of inputs
    float output2 = mixer.mix(0.3f, 0.1f, 0.5f);
    
    // Mix sum of inputs
    float outputSum = mixer.mix(0.2f + 0.3f, 0.4f + 0.1f, 0.1f + 0.5f);
    
    // Should satisfy linearity: mix(a+b) = mix(a) + mix(b)
    EXPECT_NEAR(outputSum, output1 + output2, 1e-6f);
}

// Test edge case: maximum positive values
TEST_F(GeneratorMixerTest, MaximumPositiveValues) {
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(1.0f);
    
    float output = mixer.mix(1.0f, 1.0f, 1.0f);
    EXPECT_FLOAT_EQ(output, 3.0f);
}

// Test edge case: maximum negative values
TEST_F(GeneratorMixerTest, MaximumNegativeValues) {
    mixer.setSineLevel(1.0f);
    mixer.setHarmonicLevel(1.0f);
    mixer.setNoiseLevel(1.0f);
    
    float output = mixer.mix(-1.0f, -1.0f, -1.0f);
    EXPECT_FLOAT_EQ(output, -3.0f);
}

// Test that level changes take effect immediately
TEST_F(GeneratorMixerTest, LevelChangesAreImmediate) {
    mixer.setSineLevel(0.5f);
    float output1 = mixer.mix(1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(output1, 0.5f);
    
    mixer.setSineLevel(0.8f);
    float output2 = mixer.mix(1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(output2, 0.8f);
}

// Test independent level control - changing one level doesn't affect others
TEST_F(GeneratorMixerTest, IndependentLevelControl) {
    mixer.setSineLevel(0.5f);
    mixer.setHarmonicLevel(0.3f);
    mixer.setNoiseLevel(0.7f);
    
    // Change sine level
    mixer.setSineLevel(0.9f);
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 0.9f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 0.3f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 0.7f);
    
    // Change harmonic level
    mixer.setHarmonicLevel(0.1f);
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 0.9f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 0.1f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 0.7f);
    
    // Change noise level
    mixer.setNoiseLevel(0.4f);
    EXPECT_FLOAT_EQ(mixer.getSineLevel(), 0.9f);
    EXPECT_FLOAT_EQ(mixer.getHarmonicLevel(), 0.1f);
    EXPECT_FLOAT_EQ(mixer.getNoiseLevel(), 0.4f);
}
