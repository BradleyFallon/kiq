#include <gtest/gtest.h>

#include "audio_engine/generators/TransientGenerator.h"

#include <algorithm>
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
        if (timeMs >= kDefaultKickParams.transient.airDecayMs) {
            EXPECT_FLOAT_EQ(value, 0.0f);
        }
    }
}

TEST(TransientGeneratorTest, ImpactCanBeAuditionedWithoutAir) {
    TransientGenerator transient;
    transient.initialize(48000.0f);
    transient.setParams({0.25f, 0.0f, 7.0f, 6500.0f});
    transient.trigger();

    std::vector<float> impact(96);
    for (std::size_t sample = 0; sample < impact.size(); ++sample) {
        impact[sample] = transient.renderSample(
            static_cast<float>(sample) * 1000.0f / 48000.0f);
    }

    EXPECT_FLOAT_EQ(impact.front(), 0.0f);
    EXPECT_GT(*std::max_element(impact.begin(), impact.end()), 0.05f);
    EXPECT_LT(*std::min_element(impact.begin(), impact.end()), -0.05f);
    float maximumStep = 0.0f;
    for (std::size_t sample = 1; sample < impact.size(); ++sample) {
        maximumStep =
            std::max(maximumStep, std::abs(impact[sample] - impact[sample - 1]));
    }
    EXPECT_LT(maximumStep, 0.15f);
}

TEST(TransientGeneratorTest, ImpactDurationIsSampleRateIndependent) {
    auto lastActiveTime = [](float sampleRate) {
        TransientGenerator transient;
        transient.initialize(sampleRate);
        transient.setParams({0.25f, 0.0f, 7.0f, 6500.0f});
        transient.trigger();

        float lastTimeMs = 0.0f;
        const int samples = static_cast<int>(sampleRate * 0.005f);
        for (int sample = 0; sample < samples; ++sample) {
            const float timeMs = static_cast<float>(sample) * 1000.0f / sampleRate;
            if (std::abs(transient.renderSample(timeMs)) > 1.0e-6f) {
                lastTimeMs = timeMs;
            }
        }
        return lastTimeMs;
    };

    const float time48 = lastActiveTime(48000.0f);
    const float time96 = lastActiveTime(96000.0f);
    EXPECT_GT(time48, 0.1f);
    EXPECT_LT(time48, 1.0f);
    EXPECT_NEAR(time48, time96, 1000.0f / 48000.0f);
}
