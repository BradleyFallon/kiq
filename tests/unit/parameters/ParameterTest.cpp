#include <gtest/gtest.h>
#include "audio_engine/parameters/Parameter.h"

using namespace KickDrum;

// Test fixture for Parameter tests
class ParameterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test parameter: Base Pitch (20Hz to 200Hz, default 50Hz)
        basePitch = Parameter("basePitch", "Base Pitch", 50.0f, 20.0f, 200.0f, "Hz");
        
        // Create a percentage parameter (0% to 100%, default 50%)
        level = Parameter("level", "Level", 50.0f, 0.0f, 100.0f, "%");
    }

    Parameter basePitch;
    Parameter level;
};

// Test constructor and getters
TEST_F(ParameterTest, ConstructorInitializesCorrectly) {
    EXPECT_EQ(basePitch.getId(), "basePitch");
    EXPECT_EQ(basePitch.getName(), "Base Pitch");
    EXPECT_FLOAT_EQ(basePitch.getValue(), 50.0f);
    EXPECT_FLOAT_EQ(basePitch.getDefaultValue(), 50.0f);
    EXPECT_FLOAT_EQ(basePitch.getMinValue(), 20.0f);
    EXPECT_FLOAT_EQ(basePitch.getMaxValue(), 200.0f);
    EXPECT_EQ(basePitch.getUnit(), "Hz");
}

// Test default constructor
TEST_F(ParameterTest, DefaultConstructor) {
    Parameter param;
    EXPECT_EQ(param.getId(), "");
    EXPECT_EQ(param.getName(), "");
    EXPECT_FLOAT_EQ(param.getValue(), 0.0f);
    EXPECT_FLOAT_EQ(param.getDefaultValue(), 0.0f);
    EXPECT_FLOAT_EQ(param.getMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(param.getMaxValue(), 1.0f);
    EXPECT_EQ(param.getUnit(), "");
}

// Test setValue and getValue
TEST_F(ParameterTest, SetAndGetValue) {
    basePitch.setValue(100.0f);
    EXPECT_FLOAT_EQ(basePitch.getValue(), 100.0f);
    
    level.setValue(75.0f);
    EXPECT_FLOAT_EQ(level.getValue(), 75.0f);
}

// Test value clamping to minimum
TEST_F(ParameterTest, ValueClampingToMinimum) {
    basePitch.setValue(10.0f); // Below minimum of 20Hz
    EXPECT_FLOAT_EQ(basePitch.getValue(), 20.0f);
    
    basePitch.setValue(-50.0f); // Negative value
    EXPECT_FLOAT_EQ(basePitch.getValue(), 20.0f);
}

// Test value clamping to maximum
TEST_F(ParameterTest, ValueClampingToMaximum) {
    basePitch.setValue(250.0f); // Above maximum of 200Hz
    EXPECT_FLOAT_EQ(basePitch.getValue(), 200.0f);
    
    basePitch.setValue(1000.0f); // Way above maximum
    EXPECT_FLOAT_EQ(basePitch.getValue(), 200.0f);
}

// Test value within valid range
TEST_F(ParameterTest, ValueWithinRange) {
    basePitch.setValue(100.0f);
    EXPECT_FLOAT_EQ(basePitch.getValue(), 100.0f);
    
    basePitch.setValue(20.0f); // Minimum
    EXPECT_FLOAT_EQ(basePitch.getValue(), 20.0f);
    
    basePitch.setValue(200.0f); // Maximum
    EXPECT_FLOAT_EQ(basePitch.getValue(), 200.0f);
}

// Test normalization
TEST_F(ParameterTest, Normalization) {
    // Test at default value (50Hz)
    basePitch.setValue(50.0f);
    float normalized = basePitch.normalize();
    // (50 - 20) / (200 - 20) = 30 / 180 = 0.1667
    EXPECT_NEAR(normalized, 0.1667f, 0.001f);
    
    // Test at minimum (20Hz)
    basePitch.setValue(20.0f);
    EXPECT_FLOAT_EQ(basePitch.normalize(), 0.0f);
    
    // Test at maximum (200Hz)
    basePitch.setValue(200.0f);
    EXPECT_FLOAT_EQ(basePitch.normalize(), 1.0f);
    
    // Test at midpoint (110Hz)
    basePitch.setValue(110.0f);
    // (110 - 20) / (200 - 20) = 90 / 180 = 0.5
    EXPECT_FLOAT_EQ(basePitch.normalize(), 0.5f);
}

// Test denormalization
TEST_F(ParameterTest, Denormalization) {
    // Test denormalize from 0.0 (should give minimum)
    basePitch.denormalize(0.0f);
    EXPECT_FLOAT_EQ(basePitch.getValue(), 20.0f);
    
    // Test denormalize from 1.0 (should give maximum)
    basePitch.denormalize(1.0f);
    EXPECT_FLOAT_EQ(basePitch.getValue(), 200.0f);
    
    // Test denormalize from 0.5 (should give midpoint)
    basePitch.denormalize(0.5f);
    // 20 + 0.5 * (200 - 20) = 20 + 90 = 110
    EXPECT_FLOAT_EQ(basePitch.getValue(), 110.0f);
    
    // Test denormalize from 0.1667 (should give ~50Hz)
    basePitch.denormalize(0.1667f);
    EXPECT_NEAR(basePitch.getValue(), 50.0f, 0.1f);
}

// Test denormalization clamping
TEST_F(ParameterTest, DenormalizationClamping) {
    // Test with value above 1.0
    basePitch.denormalize(1.5f);
    EXPECT_FLOAT_EQ(basePitch.getValue(), 200.0f); // Should clamp to max
    
    // Test with negative value
    basePitch.denormalize(-0.5f);
    EXPECT_FLOAT_EQ(basePitch.getValue(), 20.0f); // Should clamp to min
}

// Test round-trip normalization/denormalization
TEST_F(ParameterTest, NormalizationRoundTrip) {
    basePitch.setValue(75.0f);
    float normalized = basePitch.normalize();
    
    Parameter temp("temp", "Temp", 50.0f, 20.0f, 200.0f, "Hz");
    temp.denormalize(normalized);
    
    EXPECT_NEAR(temp.getValue(), 75.0f, 0.001f);
}

// Test reset functionality
TEST_F(ParameterTest, Reset) {
    basePitch.setValue(150.0f);
    EXPECT_FLOAT_EQ(basePitch.getValue(), 150.0f);
    
    basePitch.reset();
    EXPECT_FLOAT_EQ(basePitch.getValue(), 50.0f); // Should return to default
}

// Test isDefault
TEST_F(ParameterTest, IsDefault) {
    EXPECT_TRUE(basePitch.isDefault()); // Should start at default
    
    basePitch.setValue(100.0f);
    EXPECT_FALSE(basePitch.isDefault());
    
    basePitch.reset();
    EXPECT_TRUE(basePitch.isDefault());
    
    // Test with value very close to default (within epsilon)
    basePitch.setValue(50.0f + 1e-7f);
    EXPECT_TRUE(basePitch.isDefault());
}

// Test edge case: min == max
TEST_F(ParameterTest, MinEqualsMax) {
    Parameter fixed("fixed", "Fixed", 5.0f, 5.0f, 5.0f, "");
    
    EXPECT_FLOAT_EQ(fixed.getValue(), 5.0f);
    EXPECT_FLOAT_EQ(fixed.normalize(), 0.0f); // Should return 0 to avoid division by zero
    
    fixed.denormalize(0.5f);
    EXPECT_FLOAT_EQ(fixed.getValue(), 5.0f); // Should remain at fixed value
}

// Test edge case: min > max (should swap)
TEST_F(ParameterTest, MinGreaterThanMax) {
    Parameter swapped("swapped", "Swapped", 50.0f, 100.0f, 0.0f, "");
    
    // Constructor should swap min and max
    EXPECT_FLOAT_EQ(swapped.getMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(swapped.getMaxValue(), 100.0f);
    EXPECT_FLOAT_EQ(swapped.getValue(), 50.0f); // Default should be clamped
}

// Test edge case: default value outside range
TEST_F(ParameterTest, DefaultOutsideRange) {
    Parameter clamped("clamped", "Clamped", 300.0f, 20.0f, 200.0f, "Hz");
    
    // Default should be clamped to max
    EXPECT_FLOAT_EQ(clamped.getDefaultValue(), 200.0f);
    EXPECT_FLOAT_EQ(clamped.getValue(), 200.0f);
}

// Test with percentage parameter
TEST_F(ParameterTest, PercentageParameter) {
    level.setValue(25.0f);
    EXPECT_FLOAT_EQ(level.getValue(), 25.0f);
    EXPECT_FLOAT_EQ(level.normalize(), 0.25f);
    
    level.denormalize(0.75f);
    EXPECT_FLOAT_EQ(level.getValue(), 75.0f);
}

// Test with negative range
TEST_F(ParameterTest, NegativeRange) {
    Parameter db("threshold", "Threshold", -12.0f, -60.0f, 0.0f, "dB");
    
    EXPECT_FLOAT_EQ(db.getValue(), -12.0f);
    
    // Test normalization
    // (-12 - (-60)) / (0 - (-60)) = 48 / 60 = 0.8
    EXPECT_FLOAT_EQ(db.normalize(), 0.8f);
    
    // Test denormalization
    db.denormalize(0.5f);
    // -60 + 0.5 * (0 - (-60)) = -60 + 30 = -30
    EXPECT_FLOAT_EQ(db.getValue(), -30.0f);
}

// Test with very small range
TEST_F(ParameterTest, SmallRange) {
    Parameter fine("fine", "Fine Tune", 0.0f, -0.1f, 0.1f, "");
    
    fine.setValue(0.05f);
    EXPECT_FLOAT_EQ(fine.getValue(), 0.05f);
    
    // (0.05 - (-0.1)) / (0.1 - (-0.1)) = 0.15 / 0.2 = 0.75
    EXPECT_NEAR(fine.normalize(), 0.75f, 0.001f);
}

// Test with large range
TEST_F(ParameterTest, LargeRange) {
    Parameter wide("wide", "Wide Range", 5000.0f, 0.0f, 10000.0f, "");
    
    wide.setValue(7500.0f);
    EXPECT_FLOAT_EQ(wide.getValue(), 7500.0f);
    EXPECT_FLOAT_EQ(wide.normalize(), 0.75f);
    
    wide.denormalize(0.25f);
    EXPECT_FLOAT_EQ(wide.getValue(), 2500.0f);
}
