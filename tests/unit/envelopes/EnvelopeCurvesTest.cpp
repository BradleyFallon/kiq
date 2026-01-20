#include <gtest/gtest.h>
#include "audio_engine/envelopes/EnvelopeCurves.h"
#include <cmath>

using namespace KickDrum;

// Test fixture for envelope curves
class EnvelopeCurvesTest : public ::testing::Test {
protected:
    // Helper function to check if two floats are approximately equal
    bool approxEqual(float a, float b, float epsilon = 0.0001f) {
        return std::abs(a - b) < epsilon;
    }
};

// ============================================================================
// Linear Curve Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, LinearCurveAtZero) {
    EXPECT_FLOAT_EQ(applyLinearCurve(0.0f), 0.0f);
}

TEST_F(EnvelopeCurvesTest, LinearCurveAtOne) {
    EXPECT_FLOAT_EQ(applyLinearCurve(1.0f), 1.0f);
}

TEST_F(EnvelopeCurvesTest, LinearCurveAtHalf) {
    EXPECT_FLOAT_EQ(applyLinearCurve(0.5f), 0.5f);
}

TEST_F(EnvelopeCurvesTest, LinearCurveIsIdentity) {
    // Linear curve should return input unchanged
    for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
        EXPECT_FLOAT_EQ(applyLinearCurve(t), t);
    }
}

// ============================================================================
// Exponential Curve Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, ExponentialCurveAtZero) {
    EXPECT_FLOAT_EQ(applyExponentialCurve(0.0f), 0.0f);
}

TEST_F(EnvelopeCurvesTest, ExponentialCurveAtOne) {
    EXPECT_FLOAT_EQ(applyExponentialCurve(1.0f), 1.0f);
}

TEST_F(EnvelopeCurvesTest, ExponentialCurveAtHalf) {
    // t^2 at t=0.5 should be 0.25
    EXPECT_FLOAT_EQ(applyExponentialCurve(0.5f), 0.25f);
}

TEST_F(EnvelopeCurvesTest, ExponentialCurveBelowLinear) {
    // Exponential curve (t^2) should be below linear for t in (0, 1)
    for (float t = 0.1f; t < 1.0f; t += 0.1f) {
        EXPECT_LT(applyExponentialCurve(t), t);
    }
}

TEST_F(EnvelopeCurvesTest, ExponentialCurveIsMonotonic) {
    // Exponential curve should be monotonically increasing
    float prev = applyExponentialCurve(0.0f);
    for (float t = 0.1f; t <= 1.0f; t += 0.1f) {
        float current = applyExponentialCurve(t);
        EXPECT_GT(current, prev);
        prev = current;
    }
}

// ============================================================================
// Logarithmic Curve Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, LogarithmicCurveAtZero) {
    EXPECT_FLOAT_EQ(applyLogarithmicCurve(0.0f), 0.0f);
}

TEST_F(EnvelopeCurvesTest, LogarithmicCurveAtOne) {
    EXPECT_FLOAT_EQ(applyLogarithmicCurve(1.0f), 1.0f);
}

TEST_F(EnvelopeCurvesTest, LogarithmicCurveAtHalf) {
    // sqrt(0.5) ≈ 0.707
    EXPECT_TRUE(approxEqual(applyLogarithmicCurve(0.5f), 0.707107f));
}

TEST_F(EnvelopeCurvesTest, LogarithmicCurveAboveLinear) {
    // Logarithmic curve (sqrt(t)) should be above linear for t in (0, 1)
    for (float t = 0.1f; t < 1.0f; t += 0.1f) {
        EXPECT_GT(applyLogarithmicCurve(t), t);
    }
}

TEST_F(EnvelopeCurvesTest, LogarithmicCurveIsMonotonic) {
    // Logarithmic curve should be monotonically increasing
    float prev = applyLogarithmicCurve(0.0f);
    for (float t = 0.1f; t <= 1.0f; t += 0.1f) {
        float current = applyLogarithmicCurve(t);
        EXPECT_GT(current, prev);
        prev = current;
    }
}

// ============================================================================
// Custom Curve Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, CustomCurveAtZero) {
    EXPECT_FLOAT_EQ(applyCustomCurve(0.0f), 0.0f);
}

TEST_F(EnvelopeCurvesTest, CustomCurveAtOne) {
    EXPECT_FLOAT_EQ(applyCustomCurve(1.0f), 1.0f);
}

TEST_F(EnvelopeCurvesTest, CustomCurveAtHalf) {
    // t^3 at t=0.5 should be 0.125
    EXPECT_FLOAT_EQ(applyCustomCurve(0.5f), 0.125f);
}

TEST_F(EnvelopeCurvesTest, CustomCurveIsMonotonic) {
    // Custom curve should be monotonically increasing
    float prev = applyCustomCurve(0.0f);
    for (float t = 0.1f; t <= 1.0f; t += 0.1f) {
        float current = applyCustomCurve(t);
        EXPECT_GT(current, prev);
        prev = current;
    }
}

// ============================================================================
// applyCurve() Dispatcher Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, ApplyCurveLinear) {
    float t = 0.5f;
    EXPECT_FLOAT_EQ(applyCurve(t, CurveType::LINEAR), applyLinearCurve(t));
}

TEST_F(EnvelopeCurvesTest, ApplyCurveExponential) {
    float t = 0.5f;
    EXPECT_FLOAT_EQ(applyCurve(t, CurveType::EXPONENTIAL), applyExponentialCurve(t));
}

TEST_F(EnvelopeCurvesTest, ApplyCurveLogarithmic) {
    float t = 0.5f;
    EXPECT_FLOAT_EQ(applyCurve(t, CurveType::LOGARITHMIC), applyLogarithmicCurve(t));
}

TEST_F(EnvelopeCurvesTest, ApplyCurveCustom) {
    float t = 0.5f;
    EXPECT_FLOAT_EQ(applyCurve(t, CurveType::CUSTOM), applyCustomCurve(t));
}

// ============================================================================
// Input Clamping Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, ApplyCurveClampsBelowZero) {
    // Values below 0 should be clamped to 0
    EXPECT_FLOAT_EQ(applyCurve(-0.5f, CurveType::LINEAR), 0.0f);
    EXPECT_FLOAT_EQ(applyCurve(-1.0f, CurveType::EXPONENTIAL), 0.0f);
}

TEST_F(EnvelopeCurvesTest, ApplyCurveClampAboveOne) {
    // Values above 1 should be clamped to 1
    EXPECT_FLOAT_EQ(applyCurve(1.5f, CurveType::LINEAR), 1.0f);
    EXPECT_FLOAT_EQ(applyCurve(2.0f, CurveType::EXPONENTIAL), 1.0f);
}

// ============================================================================
// Curve Ordering Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, CurveOrdering) {
    // At t=0.5, the curves should be ordered:
    // Custom (0.125) < Exponential (0.25) < Linear (0.5) < Logarithmic (0.707)
    float t = 0.5f;
    float custom = applyCustomCurve(t);
    float exponential = applyExponentialCurve(t);
    float linear = applyLinearCurve(t);
    float logarithmic = applyLogarithmicCurve(t);
    
    EXPECT_LT(custom, exponential);
    EXPECT_LT(exponential, linear);
    EXPECT_LT(linear, logarithmic);
}

// ============================================================================
// Boundary Behavior Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, AllCurvesStartAtZero) {
    // All curves should start at 0 when t=0
    EXPECT_FLOAT_EQ(applyCurve(0.0f, CurveType::LINEAR), 0.0f);
    EXPECT_FLOAT_EQ(applyCurve(0.0f, CurveType::EXPONENTIAL), 0.0f);
    EXPECT_FLOAT_EQ(applyCurve(0.0f, CurveType::LOGARITHMIC), 0.0f);
    EXPECT_FLOAT_EQ(applyCurve(0.0f, CurveType::CUSTOM), 0.0f);
}

TEST_F(EnvelopeCurvesTest, AllCurvesEndAtOne) {
    // All curves should end at 1 when t=1
    EXPECT_FLOAT_EQ(applyCurve(1.0f, CurveType::LINEAR), 1.0f);
    EXPECT_FLOAT_EQ(applyCurve(1.0f, CurveType::EXPONENTIAL), 1.0f);
    EXPECT_FLOAT_EQ(applyCurve(1.0f, CurveType::LOGARITHMIC), 1.0f);
    EXPECT_FLOAT_EQ(applyCurve(1.0f, CurveType::CUSTOM), 1.0f);
}

TEST_F(EnvelopeCurvesTest, AllCurvesInValidRange) {
    // All curves should output values in [0.0, 1.0] for input in [0.0, 1.0]
    for (float t = 0.0f; t <= 1.0f; t += 0.05f) {
        EXPECT_GE(applyCurve(t, CurveType::LINEAR), 0.0f);
        EXPECT_LE(applyCurve(t, CurveType::LINEAR), 1.0f);
        
        EXPECT_GE(applyCurve(t, CurveType::EXPONENTIAL), 0.0f);
        EXPECT_LE(applyCurve(t, CurveType::EXPONENTIAL), 1.0f);
        
        EXPECT_GE(applyCurve(t, CurveType::LOGARITHMIC), 0.0f);
        EXPECT_LE(applyCurve(t, CurveType::LOGARITHMIC), 1.0f);
        
        EXPECT_GE(applyCurve(t, CurveType::CUSTOM), 0.0f);
        EXPECT_LE(applyCurve(t, CurveType::CUSTOM), 1.0f);
    }
}

// ============================================================================
// Practical Use Case Tests
// ============================================================================

TEST_F(EnvelopeCurvesTest, ExponentialGoodForAttack) {
    // Exponential curve should provide smooth, gradual attack
    // It should stay low for most of the time, then rise quickly
    EXPECT_LT(applyExponentialCurve(0.7f), 0.7f);  // Still below linear at 70%
    EXPECT_GT(applyExponentialCurve(0.9f), 0.8f);  // Rises quickly near end
}

TEST_F(EnvelopeCurvesTest, LogarithmicGoodForDecay) {
    // Logarithmic curve should provide natural decay
    // It should drop quickly at first, then slowly approach zero
    EXPECT_GT(applyLogarithmicCurve(0.3f), 0.3f);  // Drops quickly at start
    EXPECT_LT(applyLogarithmicCurve(0.3f), 0.7f);  // But not too quickly
}

