#include <gtest/gtest.h>

#include "audio_engine/generators/TransientGenerator.h"

#include <cmath>
#include <vector>

using namespace KickDrum;

TEST(TransientGeneratorTest, RepeatedTriggersAreSampleIdentical) {
    TransientGenerator transient;
    transient.initialize(48000.0f);
    transient.setParams(kDefaultKickParams.transient);

    auto render = [&transient]() {
        transient.trigger();
        std::vector<float> result(512);
        for (std::size_t sample = 0; sample < result.size(); ++sample) {
            result[sample] = transient.renderSample(
                static_cast<float>(sample) * 1000.0f / 48000.0f);
        }
        return result;
    };

    EXPECT_EQ(render(), render());
}

TEST(TransientGeneratorTest, TransientEndsAtConfiguredDecay) {
    TransientGenerator transient;
    transient.initialize(48000.0f);
    transient.setParams(kDefaultKickParams.transient);
    transient.trigger();

    for (int sample = 0; sample < 480; ++sample) {
        const float timeMs = static_cast<float>(sample) * 1000.0f / 48000.0f;
        const float value = transient.renderSample(timeMs);
        if (timeMs >= kDefaultKickParams.transient.noiseDecayMs) {
            EXPECT_FLOAT_EQ(value, 0.0f);
        }
    }
}

TEST(TransientGeneratorTest, ClickCanBeAuditionedWithoutNoise) {
    TransientGenerator transient;
    transient.initialize(48000.0f);
    transient.setParams({0.25f, 0.0f, 7.0f, 6500.0f});
    transient.trigger();

    EXPECT_FLOAT_EQ(transient.renderSample(0.0f), 0.25f);
    EXPECT_FLOAT_EQ(transient.renderSample(1000.0f / 48000.0f), -0.125f);
    EXPECT_FLOAT_EQ(transient.renderSample(2000.0f / 48000.0f), 0.0f);
}
