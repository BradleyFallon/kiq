#include <gtest/gtest.h>
#include "audio_engine/parameters/ParameterManager.h"

using namespace KickDrum;

// Test fixture for ParameterManager tests
class ParameterManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<ParameterManager>();
    }

    std::unique_ptr<ParameterManager> manager;
};

// Test basic parameter registration
TEST_F(ParameterManagerTest, RegisterParameter) {
    Parameter param("testParam", "Test Parameter", 50.0f, 0.0f, 100.0f, "%");
    
    EXPECT_TRUE(manager->registerParameter(param));
    EXPECT_TRUE(manager->hasParameter("testParam"));
    EXPECT_EQ(manager->getParameterCount(), 1);
}

// Test duplicate parameter registration
TEST_F(ParameterManagerTest, RegisterDuplicateParameter) {
    Parameter param1("testParam", "Test Parameter 1", 50.0f, 0.0f, 100.0f, "%");
    Parameter param2("testParam", "Test Parameter 2", 75.0f, 0.0f, 100.0f, "%");
    
    EXPECT_TRUE(manager->registerParameter(param1));
    EXPECT_FALSE(manager->registerParameter(param2)); // Should fail - duplicate ID
    EXPECT_EQ(manager->getParameterCount(), 1);
    
    // First parameter should still be registered
    EXPECT_FLOAT_EQ(manager->getParameterValue("testParam"), 50.0f);
}

// Test getParameter
TEST_F(ParameterManagerTest, GetParameter) {
    Parameter param("testParam", "Test Parameter", 50.0f, 0.0f, 100.0f, "%");
    manager->registerParameter(param);
    
    Parameter* retrieved = manager->getParameter("testParam");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getId(), "testParam");
    EXPECT_FLOAT_EQ(retrieved->getValue(), 50.0f);
    
    // Test non-existent parameter
    Parameter* notFound = manager->getParameter("nonExistent");
    EXPECT_EQ(notFound, nullptr);
}

// Test const getParameter
TEST_F(ParameterManagerTest, GetParameterConst) {
    Parameter param("testParam", "Test Parameter", 50.0f, 0.0f, 100.0f, "%");
    manager->registerParameter(param);
    
    const ParameterManager* constManager = manager.get();
    const Parameter* retrieved = constManager->getParameter("testParam");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getId(), "testParam");
}

// Test setParameterValue and getParameterValue
TEST_F(ParameterManagerTest, SetAndGetParameterValue) {
    Parameter param("testParam", "Test Parameter", 50.0f, 0.0f, 100.0f, "%");
    manager->registerParameter(param);
    
    EXPECT_TRUE(manager->setParameterValue("testParam", 75.0f));
    EXPECT_FLOAT_EQ(manager->getParameterValue("testParam"), 75.0f);
    
    // Test with non-existent parameter
    EXPECT_FALSE(manager->setParameterValue("nonExistent", 50.0f));
    EXPECT_FLOAT_EQ(manager->getParameterValue("nonExistent", 99.0f), 99.0f); // Should return default
}

// Test normalized parameter access
TEST_F(ParameterManagerTest, NormalizedParameterAccess) {
    Parameter param("testParam", "Test Parameter", 50.0f, 0.0f, 100.0f, "%");
    manager->registerParameter(param);
    
    // Set to 0.0 (normalized) = 0.0 (actual)
    EXPECT_TRUE(manager->setParameterNormalized("testParam", 0.0f));
    EXPECT_FLOAT_EQ(manager->getParameterValue("testParam"), 0.0f);
    EXPECT_FLOAT_EQ(manager->getParameterNormalized("testParam"), 0.0f);
    
    // Set to 1.0 (normalized) = 100.0 (actual)
    EXPECT_TRUE(manager->setParameterNormalized("testParam", 1.0f));
    EXPECT_FLOAT_EQ(manager->getParameterValue("testParam"), 100.0f);
    EXPECT_FLOAT_EQ(manager->getParameterNormalized("testParam"), 1.0f);
    
    // Set to 0.5 (normalized) = 50.0 (actual)
    EXPECT_TRUE(manager->setParameterNormalized("testParam", 0.5f));
    EXPECT_FLOAT_EQ(manager->getParameterValue("testParam"), 50.0f);
    EXPECT_FLOAT_EQ(manager->getParameterNormalized("testParam"), 0.5f);
    
    // Test with non-existent parameter
    EXPECT_FALSE(manager->setParameterNormalized("nonExistent", 0.5f));
    EXPECT_FLOAT_EQ(manager->getParameterNormalized("nonExistent", 0.99f), 0.99f);
}

// Test hasParameter
TEST_F(ParameterManagerTest, HasParameter) {
    Parameter param("testParam", "Test Parameter", 50.0f, 0.0f, 100.0f, "%");
    manager->registerParameter(param);
    
    EXPECT_TRUE(manager->hasParameter("testParam"));
    EXPECT_FALSE(manager->hasParameter("nonExistent"));
}

// Test getParameterCount
TEST_F(ParameterManagerTest, GetParameterCount) {
    EXPECT_EQ(manager->getParameterCount(), 0);
    
    manager->registerParameter(Parameter("param1", "Param 1", 50.0f, 0.0f, 100.0f, "%"));
    EXPECT_EQ(manager->getParameterCount(), 1);
    
    manager->registerParameter(Parameter("param2", "Param 2", 50.0f, 0.0f, 100.0f, "%"));
    EXPECT_EQ(manager->getParameterCount(), 2);
    
    manager->registerParameter(Parameter("param3", "Param 3", 50.0f, 0.0f, 100.0f, "%"));
    EXPECT_EQ(manager->getParameterCount(), 3);
}

// Test getParameterIds
TEST_F(ParameterManagerTest, GetParameterIds) {
    manager->registerParameter(Parameter("param1", "Param 1", 50.0f, 0.0f, 100.0f, "%"));
    manager->registerParameter(Parameter("param2", "Param 2", 50.0f, 0.0f, 100.0f, "%"));
    manager->registerParameter(Parameter("param3", "Param 3", 50.0f, 0.0f, 100.0f, "%"));
    
    std::vector<std::string> ids = manager->getParameterIds();
    EXPECT_EQ(ids.size(), 3);
    
    // Check that all IDs are present (order may vary due to map)
    EXPECT_NE(std::find(ids.begin(), ids.end(), "param1"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "param2"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "param3"), ids.end());
}

// Test resetParameter
TEST_F(ParameterManagerTest, ResetParameter) {
    Parameter param("testParam", "Test Parameter", 50.0f, 0.0f, 100.0f, "%");
    manager->registerParameter(param);
    
    manager->setParameterValue("testParam", 75.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("testParam"), 75.0f);
    
    EXPECT_TRUE(manager->resetParameter("testParam"));
    EXPECT_FLOAT_EQ(manager->getParameterValue("testParam"), 50.0f);
    
    // Test with non-existent parameter
    EXPECT_FALSE(manager->resetParameter("nonExistent"));
}

// Test resetAllParameters
TEST_F(ParameterManagerTest, ResetAllParameters) {
    manager->registerParameter(Parameter("param1", "Param 1", 25.0f, 0.0f, 100.0f, "%"));
    manager->registerParameter(Parameter("param2", "Param 2", 50.0f, 0.0f, 100.0f, "%"));
    manager->registerParameter(Parameter("param3", "Param 3", 75.0f, 0.0f, 100.0f, "%"));
    
    // Modify all parameters
    manager->setParameterValue("param1", 100.0f);
    manager->setParameterValue("param2", 0.0f);
    manager->setParameterValue("param3", 50.0f);
    
    // Reset all
    manager->resetAllParameters();
    
    // Check all are back to defaults
    EXPECT_FLOAT_EQ(manager->getParameterValue("param1"), 25.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("param2"), 50.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("param3"), 75.0f);
}

// Test registerAllSynthesisParameters
TEST_F(ParameterManagerTest, RegisterAllSynthesisParameters) {
    manager->registerAllSynthesisParameters();
    
    // Check that all expected parameters are registered
    EXPECT_TRUE(manager->hasParameter("basePitch"));
    EXPECT_TRUE(manager->hasParameter("sineLevel"));
    EXPECT_TRUE(manager->hasParameter("harmonicRatio"));
    EXPECT_TRUE(manager->hasParameter("harmonicLevel"));
    EXPECT_TRUE(manager->hasParameter("harmonicModDepth"));
    EXPECT_TRUE(manager->hasParameter("noiseLevel"));
    EXPECT_TRUE(manager->hasParameter("noiseModDepth"));
    
    EXPECT_TRUE(manager->hasParameter("warmUpDuration"));
    EXPECT_TRUE(manager->hasParameter("warmUpStartFreq"));
    EXPECT_TRUE(manager->hasParameter("warmUpAmplitude"));
    
    EXPECT_TRUE(manager->hasParameter("attack"));
    EXPECT_TRUE(manager->hasParameter("decay"));
    EXPECT_TRUE(manager->hasParameter("sustain"));
    EXPECT_TRUE(manager->hasParameter("release"));
    
    EXPECT_TRUE(manager->hasParameter("pitchEnvelopeDepth"));
    
    EXPECT_TRUE(manager->hasParameter("attackCurve"));
    EXPECT_TRUE(manager->hasParameter("decayCurve"));
    EXPECT_TRUE(manager->hasParameter("releaseCurve"));
    
    EXPECT_TRUE(manager->hasParameter("compressorThreshold"));
    EXPECT_TRUE(manager->hasParameter("compressorRatio"));
    EXPECT_TRUE(manager->hasParameter("compressorAttack"));
    EXPECT_TRUE(manager->hasParameter("compressorRelease"));
    EXPECT_TRUE(manager->hasParameter("compressorMix"));
    
    EXPECT_TRUE(manager->hasParameter("reverbRoomSize"));
    EXPECT_TRUE(manager->hasParameter("reverbDecayTime"));
    EXPECT_TRUE(manager->hasParameter("reverbDamping"));
    EXPECT_TRUE(manager->hasParameter("reverbMix"));
    
    EXPECT_TRUE(manager->hasParameter("masterLevel"));
    EXPECT_TRUE(manager->hasParameter("pitchTracking"));
    
    // Check parameter count (should be 31 parameters)
    EXPECT_EQ(manager->getParameterCount(), 31);
}

// Test specific synthesis parameter values
TEST_F(ParameterManagerTest, SynthesisParameterDefaults) {
    manager->registerAllSynthesisParameters();
    
    // Test some default values
    EXPECT_FLOAT_EQ(manager->getParameterValue("basePitch"), 50.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("sineLevel"), 80.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("harmonicRatio"), 2.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("attack"), 1.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("decay"), 500.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("compressorThreshold"), -12.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("masterLevel"), 80.0f);
}

// Test synthesis parameter ranges
TEST_F(ParameterManagerTest, SynthesisParameterRanges) {
    manager->registerAllSynthesisParameters();
    
    // Test base pitch range (20Hz to 200Hz)
    const Parameter* basePitch = manager->getParameter("basePitch");
    ASSERT_NE(basePitch, nullptr);
    EXPECT_FLOAT_EQ(basePitch->getMinValue(), 20.0f);
    EXPECT_FLOAT_EQ(basePitch->getMaxValue(), 200.0f);
    
    // Test harmonic ratio range (0.5x to 8.0x)
    const Parameter* harmonicRatio = manager->getParameter("harmonicRatio");
    ASSERT_NE(harmonicRatio, nullptr);
    EXPECT_FLOAT_EQ(harmonicRatio->getMinValue(), 0.5f);
    EXPECT_FLOAT_EQ(harmonicRatio->getMaxValue(), 8.0f);
    
    // Test compressor threshold range (-60dB to 0dB)
    const Parameter* threshold = manager->getParameter("compressorThreshold");
    ASSERT_NE(threshold, nullptr);
    EXPECT_FLOAT_EQ(threshold->getMinValue(), -60.0f);
    EXPECT_FLOAT_EQ(threshold->getMaxValue(), 0.0f);
}

// Test parameter modification after registration
TEST_F(ParameterManagerTest, ModifySynthesisParameters) {
    manager->registerAllSynthesisParameters();
    
    // Modify some parameters
    manager->setParameterValue("basePitch", 100.0f);
    manager->setParameterValue("sineLevel", 50.0f);
    manager->setParameterValue("harmonicRatio", 4.0f);
    
    EXPECT_FLOAT_EQ(manager->getParameterValue("basePitch"), 100.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("sineLevel"), 50.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("harmonicRatio"), 4.0f);
    
    // Reset and check
    manager->resetAllParameters();
    EXPECT_FLOAT_EQ(manager->getParameterValue("basePitch"), 50.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("sineLevel"), 80.0f);
    EXPECT_FLOAT_EQ(manager->getParameterValue("harmonicRatio"), 2.0f);
}

// Test empty manager
TEST_F(ParameterManagerTest, EmptyManager) {
    EXPECT_EQ(manager->getParameterCount(), 0);
    EXPECT_FALSE(manager->hasParameter("anything"));
    EXPECT_EQ(manager->getParameter("anything"), nullptr);
    EXPECT_TRUE(manager->getParameterIds().empty());
}
