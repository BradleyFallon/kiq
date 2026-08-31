#include <gtest/gtest.h>
#include "audio_engine/utils/DSPUtils.h"
#include <cmath>
#include <limits>
#include <vector>

using namespace KickDrum::DSPUtils;

// ============================================================================
// Soft Clipping Tests
// ============================================================================

TEST(DSPUtilsTest, SoftClipPassesThroughSmallSignals) {
    // Signals below 2/3 should pass through unchanged
    EXPECT_FLOAT_EQ(softClip(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(softClip(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(softClip(-0.5f), -0.5f);
    EXPECT_FLOAT_EQ(softClip(0.6f), 0.6f);
    EXPECT_FLOAT_EQ(softClip(-0.6f), -0.6f);
}

TEST(DSPUtilsTest, SoftClipLimitsLargeSignals) {
    // Large signals approach full scale without crossing it.
    EXPECT_GT(softClip(1.5f), 0.99f);
    EXPECT_LT(softClip(-1.5f), -0.99f);
    EXPECT_GT(softClip(2.0f), 0.999f);
    EXPECT_LT(softClip(-2.0f), -0.999f);
    EXPECT_FLOAT_EQ(softClip(10.0f), 1.0f);
    EXPECT_FLOAT_EQ(softClip(-10.0f), -1.0f);
}

TEST(DSPUtilsTest, SoftClipSmoothlyCompressesTransitionRegion) {
    // Signals between 2/3 and 1.0 should be smoothly compressed
    float input1 = 0.7f;
    float output1 = softClip(input1);
    EXPECT_GT(output1, 0.0f);
    EXPECT_LT(output1, 1.0f);
    EXPECT_GT(output1, input1 * 0.9f);  // Should be close to input
    
    float input2 = 0.9f;
    float output2 = softClip(input2);
    EXPECT_GT(output2, 0.0f);
    EXPECT_LT(output2, 1.0f);
    EXPECT_LT(output2, input2);  // Should be compressed
}

TEST(DSPUtilsTest, SoftClipIsSymmetric) {
    // Soft clipping should be symmetric around zero
    std::vector<float> testValues = {0.5f, 0.7f, 0.9f, 1.2f, 2.0f};
    
    for (float value : testValues) {
        float positive = softClip(value);
        float negative = softClip(-value);
        EXPECT_FLOAT_EQ(positive, -negative) 
            << "Asymmetry at value: " << value;
    }
}

TEST(DSPUtilsTest, SoftClipOutputAlwaysInRange) {
    // Output should always be in [-1.0, 1.0]
    std::vector<float> testValues = {
        -10.0f, -5.0f, -2.0f, -1.5f, -1.0f, -0.9f, -0.5f, 0.0f,
        0.5f, 0.9f, 1.0f, 1.5f, 2.0f, 5.0f, 10.0f
    };
    
    for (float input : testValues) {
        float output = softClip(input);
        EXPECT_GE(output, -1.0f) << "Output below -1.0 for input: " << input;
        EXPECT_LE(output, 1.0f) << "Output above 1.0 for input: " << input;
    }
}

TEST(DSPUtilsTest, SoftClipBufferProcessesAllSamples) {
    std::vector<float> buffer = {0.5f, 1.5f, -1.5f, 0.8f, -0.3f};
    std::vector<float> expected = {
        softClip(0.5f),
        softClip(1.5f),
        softClip(-1.5f),
        softClip(0.8f),
        softClip(-0.3f)
    };
    
    softClipBuffer(buffer.data(), buffer.size());
    
    for (size_t i = 0; i < buffer.size(); ++i) {
        EXPECT_FLOAT_EQ(buffer[i], expected[i]) << "Mismatch at index " << i;
    }
}

TEST(DSPUtilsTest, SoftClipHandlesEdgeCases) {
    // Test transition point (2/3)
    float transition = 2.0f / 3.0f;
    float output = softClip(transition);
    EXPECT_FLOAT_EQ(output, transition);  // Should pass through at boundary
}

TEST(DSPUtilsTest, SoftClipIsContinuousAndMonotonic) {
    const float transition = 2.0f / 3.0f;
    EXPECT_NEAR(softClip(transition - 1.0e-5f),
                softClip(transition + 1.0e-5f), 3.0e-5f);

    float previous = softClip(0.0f);
    for (int step = 1; step <= 4000; ++step) {
        const float current = softClip(static_cast<float>(step) / 1000.0f);
        EXPECT_GE(current, previous);
        previous = current;
    }
}

TEST(DSPUtilsTest, SoftClipNeverExpandsTheSignal) {
    for (int step = 0; step <= 4000; ++step) {
        const float input = static_cast<float>(step) / 1000.0f;
        EXPECT_LE(std::abs(softClip(input)), input + 1.0e-6f);
        EXPECT_LE(std::abs(softClip(-input)), input + 1.0e-6f);
    }
}

// ============================================================================
// NaN/Infinity Detection Tests
// ============================================================================

TEST(DSPUtilsTest, IsValidDetectsNaN) {
    float nan = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(isValid(nan));
}

TEST(DSPUtilsTest, IsValidDetectsInfinity) {
    float posInf = std::numeric_limits<float>::infinity();
    float negInf = -std::numeric_limits<float>::infinity();
    EXPECT_FALSE(isValid(posInf));
    EXPECT_FALSE(isValid(negInf));
}

TEST(DSPUtilsTest, IsValidAcceptsFiniteValues) {
    EXPECT_TRUE(isValid(0.0f));
    EXPECT_TRUE(isValid(1.0f));
    EXPECT_TRUE(isValid(-1.0f));
    EXPECT_TRUE(isValid(1000.0f));
    EXPECT_TRUE(isValid(-1000.0f));
    EXPECT_TRUE(isValid(0.0001f));
}

TEST(DSPUtilsTest, IsBufferValidDetectsNaN) {
    std::vector<float> buffer = {0.5f, 0.8f, std::numeric_limits<float>::quiet_NaN(), 0.3f};
    size_t invalidIndex = 0;
    
    EXPECT_FALSE(isBufferValid(buffer.data(), buffer.size(), &invalidIndex));
    EXPECT_EQ(invalidIndex, 2);
}

TEST(DSPUtilsTest, IsBufferValidDetectsInfinity) {
    std::vector<float> buffer = {0.5f, std::numeric_limits<float>::infinity(), 0.3f};
    size_t invalidIndex = 0;
    
    EXPECT_FALSE(isBufferValid(buffer.data(), buffer.size(), &invalidIndex));
    EXPECT_EQ(invalidIndex, 1);
}

TEST(DSPUtilsTest, IsBufferValidAcceptsValidBuffer) {
    std::vector<float> buffer = {0.5f, 0.8f, -0.3f, 1.0f, -1.0f};
    
    EXPECT_TRUE(isBufferValid(buffer.data(), buffer.size()));
}

TEST(DSPUtilsTest, IsBufferValidWorksWithoutIndexOutput) {
    std::vector<float> buffer = {0.5f, std::numeric_limits<float>::quiet_NaN(), 0.3f};
    
    // Should work without providing index output parameter
    EXPECT_FALSE(isBufferValid(buffer.data(), buffer.size()));
}

TEST(DSPUtilsTest, SanitizeBufferReplacesNaN) {
    std::vector<float> buffer = {0.5f, std::numeric_limits<float>::quiet_NaN(), 0.8f};
    
    size_t count = sanitizeBuffer(buffer.data(), buffer.size());
    
    EXPECT_EQ(count, 1);
    EXPECT_FLOAT_EQ(buffer[0], 0.5f);
    EXPECT_FLOAT_EQ(buffer[1], 0.0f);  // NaN replaced with 0
    EXPECT_FLOAT_EQ(buffer[2], 0.8f);
}

TEST(DSPUtilsTest, SanitizeBufferReplacesInfinity) {
    std::vector<float> buffer = {
        0.5f,
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        0.8f
    };
    
    size_t count = sanitizeBuffer(buffer.data(), buffer.size());
    
    EXPECT_EQ(count, 2);
    EXPECT_FLOAT_EQ(buffer[0], 0.5f);
    EXPECT_FLOAT_EQ(buffer[1], 0.0f);  // +inf replaced with 0
    EXPECT_FLOAT_EQ(buffer[2], 0.0f);  // -inf replaced with 0
    EXPECT_FLOAT_EQ(buffer[3], 0.8f);
}

TEST(DSPUtilsTest, SanitizeBufferReturnsZeroForValidBuffer) {
    std::vector<float> buffer = {0.5f, 0.8f, -0.3f, 1.0f};
    
    size_t count = sanitizeBuffer(buffer.data(), buffer.size());
    
    EXPECT_EQ(count, 0);
    // Buffer should be unchanged
    EXPECT_FLOAT_EQ(buffer[0], 0.5f);
    EXPECT_FLOAT_EQ(buffer[1], 0.8f);
    EXPECT_FLOAT_EQ(buffer[2], -0.3f);
    EXPECT_FLOAT_EQ(buffer[3], 1.0f);
}

TEST(DSPUtilsTest, SanitizeBufferHandlesMultipleInvalidValues) {
    std::vector<float> buffer = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN(),
        0.5f,
        -std::numeric_limits<float>::infinity()
    };
    
    size_t count = sanitizeBuffer(buffer.data(), buffer.size());
    
    EXPECT_EQ(count, 4);
    EXPECT_FLOAT_EQ(buffer[0], 0.0f);
    EXPECT_FLOAT_EQ(buffer[1], 0.0f);
    EXPECT_FLOAT_EQ(buffer[2], 0.0f);
    EXPECT_FLOAT_EQ(buffer[3], 0.5f);
    EXPECT_FLOAT_EQ(buffer[4], 0.0f);
}

// ============================================================================
// Master Level Tests
// ============================================================================

TEST(DSPUtilsTest, ApplyMasterLevelScalesBuffer) {
    std::vector<float> buffer = {0.5f, 1.0f, -0.5f, -1.0f};
    float level = 0.5f;
    
    applyMasterLevel(buffer.data(), buffer.size(), level);
    
    EXPECT_FLOAT_EQ(buffer[0], 0.25f);
    EXPECT_FLOAT_EQ(buffer[1], 0.5f);
    EXPECT_FLOAT_EQ(buffer[2], -0.25f);
    EXPECT_FLOAT_EQ(buffer[3], -0.5f);
}

TEST(DSPUtilsTest, ApplyMasterLevelWithZeroSilencesBuffer) {
    std::vector<float> buffer = {0.5f, 1.0f, -0.5f, -1.0f};
    
    applyMasterLevel(buffer.data(), buffer.size(), 0.0f);
    
    for (float sample : buffer) {
        EXPECT_FLOAT_EQ(sample, 0.0f);
    }
}

TEST(DSPUtilsTest, ApplyMasterLevelWithOnePreservesBuffer) {
    std::vector<float> original = {0.5f, 1.0f, -0.5f, -1.0f};
    std::vector<float> buffer = original;
    
    applyMasterLevel(buffer.data(), buffer.size(), 1.0f);
    
    for (size_t i = 0; i < buffer.size(); ++i) {
        EXPECT_FLOAT_EQ(buffer[i], original[i]);
    }
}

TEST(DSPUtilsTest, ApplyMasterLevelClampsNegativeLevel) {
    std::vector<float> buffer = {0.5f, 1.0f};
    
    // Negative level should be clamped to 0
    applyMasterLevel(buffer.data(), buffer.size(), -0.5f);
    
    EXPECT_FLOAT_EQ(buffer[0], 0.0f);
    EXPECT_FLOAT_EQ(buffer[1], 0.0f);
}

TEST(DSPUtilsTest, ApplyMasterLevelClampsExcessiveLevel) {
    std::vector<float> buffer = {0.5f, 1.0f};
    
    // Level > 1.0 should be clamped to 1.0
    applyMasterLevel(buffer.data(), buffer.size(), 2.0f);
    
    EXPECT_FLOAT_EQ(buffer[0], 0.5f);
    EXPECT_FLOAT_EQ(buffer[1], 1.0f);
}

TEST(DSPUtilsTest, ApplyMasterLevelPreservesSign) {
    std::vector<float> buffer = {0.5f, -0.5f, 0.8f, -0.8f};
    float level = 0.5f;
    
    applyMasterLevel(buffer.data(), buffer.size(), level);
    
    EXPECT_GT(buffer[0], 0.0f);
    EXPECT_LT(buffer[1], 0.0f);
    EXPECT_GT(buffer[2], 0.0f);
    EXPECT_LT(buffer[3], 0.0f);
}

// ============================================================================
// Utility Function Tests
// ============================================================================

TEST(DSPUtilsTest, ClampLimitsToRange) {
    EXPECT_FLOAT_EQ(clamp(0.5f, 0.0f, 1.0f), 0.5f);
    EXPECT_FLOAT_EQ(clamp(-0.5f, 0.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(clamp(1.5f, 0.0f, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(clamp(0.0f, 0.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(clamp(1.0f, 0.0f, 1.0f), 1.0f);
}

TEST(DSPUtilsTest, PreventDenormalAddsOffset) {
    float value = 0.0f;
    float result = preventDenormal(value);
    
    // Result should be non-zero but very small
    EXPECT_NE(result, 0.0f);
    EXPECT_LT(std::abs(result), 1e-20f);
}

TEST(DSPUtilsTest, PreventDenormalDoesNotSignificantlyAlterValue) {
    float value = 0.5f;
    float result = preventDenormal(value);
    
    // Result should be very close to original
    EXPECT_NEAR(result, value, 1e-20f);
}
