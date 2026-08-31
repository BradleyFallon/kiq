#include <gtest/gtest.h>

#include "audio_engine/include/AudioEngine.h"
#include "audio_engine/parameters/ParameterEventQueue.h"
#include "audio_engine/parameters/ParameterManager.h"
#include "audio_engine/voice/VoiceAllocator.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

using namespace KickDrum;

TEST(AudioEngineTest, DefaultHitProducesFiniteDeterministicStereo) {
    auto render = [] {
        AudioEngine engine;
        engine.initialize(48000.0f);
        engine.noteOn(36, 1.0f);
        std::vector<float> buffer(12000 * 2);
        engine.processBlock(buffer.data(), 12000, 2);
        return buffer;
    };

    const auto first = render();
    const auto second = render();
    EXPECT_EQ(first, second);
    EXPECT_TRUE(std::any_of(first.begin(), first.end(),
                            [](float sample) { return sample != 0.0f; }));
    for (std::size_t sample = 0; sample < first.size() / 2; ++sample) {
        EXPECT_FLOAT_EQ(first[sample * 2], first[sample * 2 + 1]);
        EXPECT_TRUE(std::isfinite(first[sample * 2]));
    }
}

TEST(AudioEngineTest, DefaultHitDoesNotReachFullScale) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.noteOn(36, 1.0f);
    std::vector<float> buffer(12000);
    engine.processBlock(buffer.data(), buffer.size(), 1);

    const auto peak = *std::max_element(
        buffer.begin(), buffer.end(),
        [](float left, float right) { return std::abs(left) < std::abs(right); });
    EXPECT_LT(std::abs(peak), 0.99f);
}

TEST(AudioEngineTest, QueuedParametersApplyBeforeQueuedAudition) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.getParameterEventQueue()->addEvent("outputGain", 0.0f, 0);
    engine.enqueueNoteOn(36, 1.0f);
    std::vector<float> buffer(512);
    engine.processBlock(buffer.data(), buffer.size(), 1);

    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(),
                            [](float sample) { return sample == 0.0f; }));
}

TEST(AudioEngineTest, ParametersUpdateAuthoritativeVoiceModel) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setParameter("pitch0Hz", 330.0f);
    engine.setParameter("outputGain", 0.5f);

    EXPECT_FLOAT_EQ(engine.getParameterManager()->getParameterValue("pitch0Hz"),
                    330.0f);
    EXPECT_FLOAT_EQ(engine.getVoiceAllocator()->getParams().pitch[0].value, 330.0f);
    EXPECT_FLOAT_EQ(engine.getOutputGain(), 0.5f);
}

TEST(AudioEngineTest, ReinitializingAtANewSampleRatePreservesParameters) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setParameter("pitch2Hz", 61.0f);
    engine.initialize(96000.0f);

    EXPECT_FLOAT_EQ(engine.getSampleRate(), 96000.0f);
    EXPECT_FLOAT_EQ(engine.getParameterManager()->getParameterValue("pitch2Hz"),
                    61.0f);
    EXPECT_FLOAT_EQ(engine.getVoiceAllocator()->getParams().pitch[2].value, 61.0f);
}

TEST(AudioEngineTest, OutputGainZeroSilencesHit) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setOutputGain(0.0f);
    engine.noteOn(36, 1.0f);
    std::vector<float> buffer(512);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(),
                            [](float sample) { return sample == 0.0f; }));
}

TEST(AudioEngineTest, QueuedNoteAndMeterCrossTheAudioThreadBoundary) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.prepare(512);
    engine.enqueueNoteOn(36, 1.0f);

    std::vector<float> buffer(512);
    engine.processBlock(buffer.data(), buffer.size(), 1);

    EXPECT_TRUE(std::any_of(buffer.begin(), buffer.end(),
                            [](float sample) { return sample != 0.0f; }));
    EXPECT_GT(engine.getOutputPeak(), 0.0f);
    EXPECT_FLOAT_EQ(engine.getOutputPeak(), 0.0f);
}

TEST(AudioEngineTest, ClipTelemetryReportsPreLimiterOverload) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setParameter("amp1Db", 6.0f);
    engine.setParameter("impactLevel", 1.0f);
    engine.setParameter("airLevel", 1.0f);
    engine.setOutputGain(1.0f);
    engine.noteOn(36, 1.0f);

    std::vector<float> buffer(2048);
    engine.processBlock(buffer.data(), buffer.size(), 1);

    EXPECT_TRUE(engine.getOutputClip());
    EXPECT_FALSE(engine.getOutputClip());
    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(), [](float sample) {
        return std::abs(sample) < 1.0f;
    }));
}

TEST(AudioEngineTest, AuditionLoopRetriggersAtExactSampleIntervals) {
    constexpr std::size_t intervalSamples = 12000;
    constexpr std::size_t renderSamples = intervalSamples * 2 + 512;

    AudioEngine referenceEngine;
    referenceEngine.initialize(48000.0f);
    referenceEngine.setSoftClippingEnabled(false);
    referenceEngine.noteOn(36, 1.0f);
    std::vector<float> reference(renderSamples);
    referenceEngine.processBlock(reference.data(), reference.size(), 1);

    AudioEngine loopEngine;
    loopEngine.initialize(48000.0f);
    loopEngine.setSoftClippingEnabled(false);
    loopEngine.setAuditionLoop(true, 240.0f);
    std::vector<float> looped(renderSamples);
    loopEngine.processBlock(looped.data(), looped.size(), 1);

    for (std::size_t sample = 0; sample < renderSamples; ++sample) {
        float expected = reference[sample];
        if (sample >= intervalSamples) {
            expected += reference[sample - intervalSamples];
        }
        if (sample >= intervalSamples * 2) {
            expected += reference[sample - intervalSamples * 2];
        }
        EXPECT_NEAR(looped[sample], expected, 1.0e-6f) << "sample " << sample;
    }
}

TEST(AudioEngineTest, DisablingAuditionLoopStopsFutureRetriggers) {
    constexpr std::size_t firstBlockSamples = 512;
    constexpr std::size_t secondBlockSamples = 30000;

    auto render = [](bool disableLoop) {
        AudioEngine engine;
        engine.initialize(48000.0f);
        engine.setSoftClippingEnabled(false);
        if (disableLoop) {
            engine.setAuditionLoop(true, 240.0f);
        } else {
            engine.noteOn(36, 1.0f);
        }

        std::vector<float> rendered(firstBlockSamples + secondBlockSamples);
        engine.processBlock(rendered.data(), firstBlockSamples, 1);
        if (disableLoop) {
            engine.setAuditionLoop(false, 240.0f);
        }
        engine.processBlock(rendered.data() + firstBlockSamples,
                            secondBlockSamples, 1);
        return rendered;
    };

    EXPECT_EQ(render(true), render(false));
}

TEST(AudioEngineTest, SampleAccurateEventChangesFollowingSamples) {
    auto makeEngine = [] {
        auto engine = std::make_unique<AudioEngine>();
        engine->initialize(48000.0f);
        engine->setSoftClippingEnabled(false);
        engine->noteOn(36, 1.0f);
        return engine;
    };

    auto referenceEngine = makeEngine();
    std::vector<float> reference(512);
    referenceEngine->processBlock(reference.data(), reference.size(), 1);

    auto engine = makeEngine();
    engine->getParameterEventQueue()->addEvent("outputGain", 0.0f, 128);
    std::vector<float> buffer(512);
    engine->processBlock(buffer.data(), buffer.size(), 1);

    EXPECT_TRUE(std::equal(buffer.begin(), buffer.begin() + 128,
                           reference.begin()));
    EXPECT_LT(std::abs(buffer[128]), std::abs(reference[128]));
    EXPECT_TRUE(std::any_of(buffer.begin() + 128, buffer.begin() + 367,
                            [](float sample) { return sample != 0.0f; }));
    EXPECT_TRUE(std::all_of(buffer.begin() + 367, buffer.end(),
                            [](float sample) { return sample == 0.0f; }));
}

TEST(AudioEngineTest, NoteOffDoesNotCutHitAndAllNotesOffDoes) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.noteOn(36, 1.0f);
    engine.noteOff(36);
    EXPECT_EQ(engine.getVoiceAllocator()->getNumActiveVoices(), 1);
    engine.allNotesOff();
    EXPECT_EQ(engine.getVoiceAllocator()->getNumActiveVoices(), 0);
}

TEST(AudioEngineTest, InvalidCallsAreHarmless) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.processBlock(nullptr, 100, 2);
    std::vector<float> buffer(10, 42.0f);
    engine.processBlock(buffer.data(), 0, 2);
    engine.processBlock(buffer.data(), 5, 0);
    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(),
                            [](float sample) { return sample == 42.0f; }));
}
