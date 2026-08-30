#include <gtest/gtest.h>

#include "audio_engine/voice/Voice.h"

#include <cmath>
#include <vector>

using namespace KickDrum;

namespace {
std::vector<float> renderHit(Voice& voice, int samples, float velocity = 1.0f) {
    voice.trigger(60, velocity);
    std::vector<float> result(static_cast<std::size_t>(samples));
    for (float& sample : result) {
        sample = voice.renderSample();
    }
    return result;
}
}

TEST(VoiceTest, RepeatedHitsResetPhaseAndRenderIdentically) {
    Voice voice;
    voice.initialize(48000.0f);

    EXPECT_EQ(renderHit(voice, 12000), renderHit(voice, 12000));
}

TEST(VoiceTest, SampleRateDoesNotChangeTrajectoryTiming) {
    Voice voice48;
    Voice voice96;
    voice48.initialize(48000.0f);
    voice96.initialize(96000.0f);
    voice48.trigger(60, 1.0f);
    voice96.trigger(60, 1.0f);

    for (int sample = 0; sample < 864; ++sample) {
        voice48.renderSample();
    }
    for (int sample = 0; sample < 1728; ++sample) {
        voice96.renderSample();
    }

    EXPECT_NEAR(voice48.getCurrentPitchHz(), 105.0f, 0.001f);
    EXPECT_NEAR(voice96.getCurrentPitchHz(), 105.0f, 0.001f);
}

TEST(VoiceTest, BodyReachesSettleFrequency) {
    Voice voice;
    voice.initialize(48000.0f);
    voice.trigger(60, 1.0f);

    for (int sample = 0; sample < 2784; ++sample) {
        voice.renderSample();
    }

    EXPECT_NEAR(voice.getCurrentPitchHz(), 52.0f, 0.001f);
}

TEST(VoiceTest, TransientIsGoneWellBeforeBodyEnds) {
    Voice withTransient;
    Voice bodyOnly;
    withTransient.initialize(48000.0f);
    bodyOnly.initialize(48000.0f);
    KickParams bodyParams = kDefaultKickParams;
    bodyParams.transient.clickLevel = 0.0f;
    bodyParams.transient.noiseLevel = 0.0f;
    bodyOnly.setParams(bodyParams);

    const auto full = renderHit(withTransient, 1200);
    const auto body = renderHit(bodyOnly, 1200);
    EXPECT_GT(std::abs(full[0] - body[0]), 0.05f);
    for (std::size_t sample = 480; sample < full.size(); ++sample) {
        EXPECT_FLOAT_EQ(full[sample], body[sample]);
    }
}

TEST(VoiceTest, NoteOffDoesNotTruncateOneShotButStopDoes) {
    Voice voice;
    voice.initialize(48000.0f);
    voice.trigger(60, 1.0f);
    voice.release();
    EXPECT_TRUE(voice.isActive());
    voice.stop();
    EXPECT_FALSE(voice.isActive());
    EXPECT_FLOAT_EQ(voice.renderSample(), 0.0f);
}

TEST(VoiceTest, FinishesAtAmplitudeTrajectoryEnd) {
    Voice voice;
    voice.initialize(48000.0f);
    voice.trigger(60, 1.0f);
    for (int sample = 0; sample < 11000; ++sample) {
        voice.renderSample();
    }
    EXPECT_FALSE(voice.isActive());
}

TEST(VoiceTest, VelocityScalesEverySample) {
    Voice full;
    Voice half;
    full.initialize(48000.0f);
    half.initialize(48000.0f);
    const auto fullBuffer = renderHit(full, 512, 1.0f);
    const auto halfBuffer = renderHit(half, 512, 0.5f);
    for (std::size_t sample = 0; sample < fullBuffer.size(); ++sample) {
        EXPECT_FLOAT_EQ(halfBuffer[sample], fullBuffer[sample] * 0.5f);
    }
}

TEST(VoiceTest, OutputStaysFinite) {
    Voice voice;
    voice.initialize(48000.0f);
    for (const float sample : renderHit(voice, 12000)) {
        EXPECT_TRUE(std::isfinite(sample));
    }
}
