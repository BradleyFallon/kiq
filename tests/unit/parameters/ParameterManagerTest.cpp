#include <gtest/gtest.h>

#include "audio_engine/parameters/KickParams.h"
#include "audio_engine/parameters/ParameterManager.h"

#include <string>

using namespace KickDrum;

TEST(ParameterManagerTest, RegistersExactlyTheAuthoritativeKickParameters) {
    ParameterManager manager;
    manager.registerAllSynthesisParameters();

    EXPECT_EQ(manager.getParameterCount(), kKickParameterSpecs.size());
    for (const auto& spec : kKickParameterSpecs) {
        const auto* parameter = manager.getParameter(std::string(spec.key));
        ASSERT_NE(parameter, nullptr) << spec.key;
        EXPECT_FLOAT_EQ(parameter->getDefaultValue(),
                        getDefaultKickParameter(spec.id));
        EXPECT_FLOAT_EQ(parameter->getMinValue(), spec.minimum);
        EXPECT_FLOAT_EQ(parameter->getMaxValue(), spec.maximum);
    }
}

TEST(ParameterManagerTest, ObsoleteTopologyParametersAreGone) {
    ParameterManager manager;
    manager.registerAllSynthesisParameters();

    EXPECT_FALSE(manager.hasParameter("basePitch"));
    EXPECT_FALSE(manager.hasParameter("pitchEnvelopeDepth"));
    EXPECT_FALSE(manager.hasParameter("harmonicRatio"));
    EXPECT_FALSE(manager.hasParameter("compressorThreshold"));
    EXPECT_FALSE(manager.hasParameter("reverbMix"));
}

TEST(ParameterManagerTest, ValuesClampAndResetToKickDefaults) {
    ParameterManager manager;
    manager.registerAllSynthesisParameters();
    manager.setParameterValue("pitch0Hz", 5000.0f);
    EXPECT_FLOAT_EQ(manager.getParameterValue("pitch0Hz"), 1000.0f);
    manager.resetAllParameters();
    EXPECT_FLOAT_EQ(manager.getParameterValue("pitch0Hz"), 220.0f);
}

TEST(ParameterManagerTest, SerializationRoundTripsCurrentModel) {
    ParameterManager source;
    source.registerAllSynthesisParameters();
    source.setParameterValue("pitch2Hz", 61.0f);
    source.setParameterValue("noiseDecayMs", 12.0f);

    ParameterManager destination;
    destination.registerAllSynthesisParameters();
    EXPECT_TRUE(destination.deserializeFromJSON(source.serializeToJSON("2.0.0")));
    EXPECT_FLOAT_EQ(destination.getParameterValue("pitch2Hz"), 61.0f);
    EXPECT_FLOAT_EQ(destination.getParameterValue("noiseDecayMs"), 12.0f);
}
