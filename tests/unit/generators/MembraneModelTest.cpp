#include <gtest/gtest.h>

#include "audio_engine/generators/MembraneModel.h"
#include "audio_engine/generators/SineDriver.h"

#include <cmath>
#include <vector>

using namespace KickDrum;

namespace {

std::vector<float> renderMembrane(MembraneModel& membrane, int samples) {
    membrane.trigger();
    std::vector<float> result(static_cast<std::size_t>(samples));
    for (int sample = 0; sample < samples; ++sample) {
        result[static_cast<std::size_t>(sample)] = membrane.renderSample(
            60.0f, static_cast<float>(sample) * 1000.0f / 48000.0f);
    }
    return result;
}

} // namespace

TEST(MembraneModelTest, RepeatedImpactsAreDeterministic) {
    MembraneModel membrane;
    membrane.initialize(48000.0f);

    EXPECT_EQ(renderMembrane(membrane, 2048), renderMembrane(membrane, 2048));
}

TEST(MembraneModelTest, StrikePositionChangesModalContent) {
    MembraneModel membrane;
    membrane.initialize(48000.0f);
    membrane.setStrikePosition(0.0f);
    const auto center = renderMembrane(membrane, 1024);
    membrane.setStrikePosition(1.0f);
    const auto edge = renderMembrane(membrane, 1024);

    EXPECT_NE(center, edge);
    EXPECT_FLOAT_EQ(membrane.getStrikePosition(), 1.0f);
}

TEST(MembraneModelTest, CenterStrikeExcitesOnlyFundamentalMode) {
    MembraneModel membrane;
    membrane.initialize(48000.0f);
    membrane.setStrikePosition(0.0f);
    const auto center = renderMembrane(membrane, 1024);

    SineDriver fundamental;
    fundamental.initialize(48000.0f);
    fundamental.setFrequency(60.0f);
    for (const float sample : center) {
        EXPECT_NEAR(sample, fundamental.generate(), 1.0e-6f);
    }
}

TEST(MembraneModelTest, FundamentalPhaseCanRotateWithoutMovingUpperModes) {
    MembraneModel membrane;
    membrane.initialize(48000.0f);
    membrane.setStrikePosition(0.0f);
    membrane.setFundamentalPhase(0.25f);
    membrane.trigger();

    EXPECT_NEAR(membrane.renderSample(60.0f, 0.0f), 1.0f, 1.0e-6f);
    membrane.setFundamentalPhase(-0.25f);
    membrane.trigger();
    EXPECT_NEAR(membrane.renderSample(60.0f, 0.0f), -1.0f, 1.0e-6f);
}
