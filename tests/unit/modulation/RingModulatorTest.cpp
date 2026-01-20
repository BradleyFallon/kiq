#include <gtest/gtest.h>
#include "audio_engine/modulation/RingModulator.h"
#include <cmath>

using namespace KickDrum;

/**
 * Test fixture for RingModulator tests
 */
class RingModulatorTest : public ::testing::Test {
protected:
    RingModulator modulator;
};

/**
 * Test: Default constructor initializes depth to 0.0
 */
TEST_F(RingModulatorTest, DefaultConstructorInitializesDepthToZero) {
    EXPECT_FLOAT_EQ(modulator.getDepth(), 0.0f);
}

/**
 * Test: setDepth correctly sets the depth value
 */
TEST_F(RingModulatorTest, SetDepthSetsCorrectValue) {
    modulator.setDepth(0.5f);
    EXPECT_FLOAT_EQ(modulator.getDepth(), 0.5f);
    
    modulator.setDepth(0.0f);
    EXPECT_FLOAT_EQ(modulator.getDepth(), 0.0f);
    
    modulator.setDepth(1.0f);
    EXPECT_FLOAT_EQ(modulator.getDepth(), 1.0f);
}

/**
 * Test: setDepth clamps values below 0.0 to 0.0
 */
TEST_F(RingModulatorTest, SetDepthClampsNegativeValues) {
    modulator.setDepth(-0.5f);
    EXPECT_FLOAT_EQ(modulator.getDepth(), 0.0f);
    
    modulator.setDepth(-1.0f);
    EXPECT_FLOAT_EQ(modulator.getDepth(), 0.0f);
}

/**
 * Test: setDepth clamps values above 1.0 to 1.0
 */
TEST_F(RingModulatorTest, SetDepthClampsValuesAboveOne) {
    modulator.setDepth(1.5f);
    EXPECT_FLOAT_EQ(modulator.getDepth(), 1.0f);
    
    modulator.setDepth(2.0f);
    EXPECT_FLOAT_EQ(modulator.getDepth(), 1.0f);
}

/**
 * Test: At 0% depth, output equals carrier (fully dry)
 * Validates: Requirements 3.4
 */
TEST_F(RingModulatorTest, ZeroDepthOutputsCarrierUnmodified) {
    modulator.setDepth(0.0f);
    
    // Test with various carrier and modulator values
    EXPECT_FLOAT_EQ(modulator.process(1.0f, 0.5f), 1.0f);
    EXPECT_FLOAT_EQ(modulator.process(0.5f, 1.0f), 0.5f);
    EXPECT_FLOAT_EQ(modulator.process(-0.5f, 0.8f), -0.5f);
    EXPECT_FLOAT_EQ(modulator.process(0.0f, 1.0f), 0.0f);
}

/**
 * Test: At 100% depth, output equals carrier × modulator (fully wet)
 * Validates: Requirements 1.5, 1.6, 3.5, 3.7
 */
TEST_F(RingModulatorTest, FullDepthOutputsRingModulation) {
    modulator.setDepth(1.0f);
    
    // Test with various carrier and modulator values
    EXPECT_FLOAT_EQ(modulator.process(1.0f, 0.5f), 0.5f);
    EXPECT_FLOAT_EQ(modulator.process(0.5f, 1.0f), 0.5f);
    EXPECT_FLOAT_EQ(modulator.process(0.8f, 0.5f), 0.4f);
    EXPECT_FLOAT_EQ(modulator.process(-0.5f, 0.8f), -0.4f);
    EXPECT_FLOAT_EQ(modulator.process(0.0f, 1.0f), 0.0f);
}

/**
 * Test: At 50% depth, output is blend of dry and wet
 */
TEST_F(RingModulatorTest, HalfDepthBlendsDryAndWet) {
    modulator.setDepth(0.5f);
    
    // carrier = 1.0, modulator = 0.5
    // dry = 1.0, wet = 0.5
    // output = 1.0 * 0.5 + 0.5 * 0.5 = 0.5 + 0.25 = 0.75
    EXPECT_FLOAT_EQ(modulator.process(1.0f, 0.5f), 0.75f);
    
    // carrier = 0.8, modulator = 0.5
    // dry = 0.8, wet = 0.4
    // output = 0.8 * 0.5 + 0.4 * 0.5 = 0.4 + 0.2 = 0.6
    EXPECT_FLOAT_EQ(modulator.process(0.8f, 0.5f), 0.6f);
}

/**
 * Test: Ring modulation with sine wave carrier and modulator
 * This simulates the actual use case with Sine Driver and Harmonic Membrane
 */
TEST_F(RingModulatorTest, RingModulationWithSineWaves) {
    modulator.setDepth(1.0f);
    
    // Simulate sine waves at different phases
    float carrier = std::sin(0.0f);      // 0.0
    float mod = std::sin(M_PI / 2.0f);   // 1.0
    EXPECT_FLOAT_EQ(modulator.process(carrier, mod), 0.0f);
    
    carrier = std::sin(M_PI / 2.0f);     // 1.0
    mod = std::sin(M_PI / 2.0f);         // 1.0
    EXPECT_FLOAT_EQ(modulator.process(carrier, mod), 1.0f);
    
    carrier = std::sin(M_PI / 4.0f);     // ~0.707
    mod = std::sin(M_PI / 4.0f);         // ~0.707
    float expected = carrier * mod;
    EXPECT_NEAR(modulator.process(carrier, mod), expected, 0.0001f);
}

/**
 * Test: Ring modulation with noise-like random values
 * This simulates the use case with Sine Driver and Noise Generator
 */
TEST_F(RingModulatorTest, RingModulationWithNoiseValues) {
    modulator.setDepth(1.0f);
    
    // Test with random-like values in [-1.0, 1.0]
    EXPECT_FLOAT_EQ(modulator.process(0.5f, 0.3f), 0.15f);
    EXPECT_FLOAT_EQ(modulator.process(0.7f, -0.4f), -0.28f);
    EXPECT_FLOAT_EQ(modulator.process(-0.6f, 0.8f), -0.48f);
    EXPECT_FLOAT_EQ(modulator.process(-0.3f, -0.5f), 0.15f);
}

/**
 * Test: Linear blending behavior across depth range
 */
TEST_F(RingModulatorTest, LinearBlendingAcrossDepthRange) {
    float carrier = 0.8f;
    float mod = 0.5f;
    float dry = carrier;
    float wet = carrier * mod;
    
    // Test at various depth values
    modulator.setDepth(0.0f);
    EXPECT_FLOAT_EQ(modulator.process(carrier, mod), dry);
    
    modulator.setDepth(0.25f);
    float expected = dry * 0.75f + wet * 0.25f;
    EXPECT_FLOAT_EQ(modulator.process(carrier, mod), expected);
    
    modulator.setDepth(0.5f);
    expected = dry * 0.5f + wet * 0.5f;
    EXPECT_FLOAT_EQ(modulator.process(carrier, mod), expected);
    
    modulator.setDepth(0.75f);
    expected = dry * 0.25f + wet * 0.75f;
    EXPECT_FLOAT_EQ(modulator.process(carrier, mod), expected);
    
    modulator.setDepth(1.0f);
    EXPECT_FLOAT_EQ(modulator.process(carrier, mod), wet);
}

/**
 * Test: Zero carrier produces zero output regardless of modulator
 */
TEST_F(RingModulatorTest, ZeroCarrierProducesZeroOutput) {
    modulator.setDepth(1.0f);
    
    EXPECT_FLOAT_EQ(modulator.process(0.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(modulator.process(0.0f, 0.5f), 0.0f);
    EXPECT_FLOAT_EQ(modulator.process(0.0f, -1.0f), 0.0f);
    
    modulator.setDepth(0.5f);
    EXPECT_FLOAT_EQ(modulator.process(0.0f, 1.0f), 0.0f);
}

/**
 * Test: Zero modulator at full depth produces zero output
 */
TEST_F(RingModulatorTest, ZeroModulatorAtFullDepthProducesZeroOutput) {
    modulator.setDepth(1.0f);
    
    EXPECT_FLOAT_EQ(modulator.process(1.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(modulator.process(0.5f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(modulator.process(-0.5f, 0.0f), 0.0f);
}

/**
 * Test: Zero modulator at zero depth outputs carrier
 */
TEST_F(RingModulatorTest, ZeroModulatorAtZeroDepthOutputsCarrier) {
    modulator.setDepth(0.0f);
    
    EXPECT_FLOAT_EQ(modulator.process(1.0f, 0.0f), 1.0f);
    EXPECT_FLOAT_EQ(modulator.process(0.5f, 0.0f), 0.5f);
    EXPECT_FLOAT_EQ(modulator.process(-0.5f, 0.0f), -0.5f);
}

/**
 * Test: Negative carrier and modulator values
 */
TEST_F(RingModulatorTest, NegativeCarrierAndModulatorValues) {
    modulator.setDepth(1.0f);
    
    // Negative × Negative = Positive
    EXPECT_FLOAT_EQ(modulator.process(-0.5f, -0.5f), 0.25f);
    EXPECT_FLOAT_EQ(modulator.process(-1.0f, -1.0f), 1.0f);
    
    // Negative × Positive = Negative
    EXPECT_FLOAT_EQ(modulator.process(-0.5f, 0.5f), -0.25f);
    EXPECT_FLOAT_EQ(modulator.process(0.5f, -0.5f), -0.25f);
}

/**
 * Test: Extreme values at boundaries
 */
TEST_F(RingModulatorTest, ExtremeValuesAtBoundaries) {
    modulator.setDepth(1.0f);
    
    // Maximum positive values
    EXPECT_FLOAT_EQ(modulator.process(1.0f, 1.0f), 1.0f);
    
    // Maximum negative values
    EXPECT_FLOAT_EQ(modulator.process(-1.0f, -1.0f), 1.0f);
    
    // Mixed extremes
    EXPECT_FLOAT_EQ(modulator.process(1.0f, -1.0f), -1.0f);
    EXPECT_FLOAT_EQ(modulator.process(-1.0f, 1.0f), -1.0f);
}

/**
 * Test: Multiple sequential process calls maintain correct state
 */
TEST_F(RingModulatorTest, SequentialProcessCallsMaintainState) {
    modulator.setDepth(0.5f);
    
    // Process multiple samples
    float result1 = modulator.process(0.8f, 0.5f);
    float result2 = modulator.process(0.6f, 0.7f);
    float result3 = modulator.process(0.4f, 0.3f);
    
    // Verify each result independently
    EXPECT_FLOAT_EQ(result1, 0.8f * 0.5f + (0.8f * 0.5f) * 0.5f);
    EXPECT_FLOAT_EQ(result2, 0.6f * 0.5f + (0.6f * 0.7f) * 0.5f);
    EXPECT_FLOAT_EQ(result3, 0.4f * 0.5f + (0.4f * 0.3f) * 0.5f);
}

/**
 * Test: Depth changes between process calls
 */
TEST_F(RingModulatorTest, DepthChangesBetweenProcessCalls) {
    float carrier = 0.8f;
    float mod = 0.5f;
    
    modulator.setDepth(0.0f);
    float result1 = modulator.process(carrier, mod);
    EXPECT_FLOAT_EQ(result1, carrier);
    
    modulator.setDepth(0.5f);
    float result2 = modulator.process(carrier, mod);
    EXPECT_FLOAT_EQ(result2, carrier * 0.5f + (carrier * mod) * 0.5f);
    
    modulator.setDepth(1.0f);
    float result3 = modulator.process(carrier, mod);
    EXPECT_FLOAT_EQ(result3, carrier * mod);
}
