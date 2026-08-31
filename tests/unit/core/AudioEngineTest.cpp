#include <gtest/gtest.h>

#include "audio_engine/include/AudioEngine.h"
#include "audio_engine/parameters/ParameterEventQueue.h"
#include "audio_engine/parameters/ParameterManager.h"
#include "audio_engine/voice/VoiceAllocator.h"

#include <algorithm>
#include <cmath>
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

TEST(AudioEngineTest, SampleAccurateEventChangesFollowingSamples) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);
    engine.noteOn(36, 1.0f);
    engine.getParameterEventQueue()->addEvent("outputGain", 0.0f, 128);
    std::vector<float> buffer(256);
    engine.processBlock(buffer.data(), buffer.size(), 1);

    EXPECT_TRUE(std::any_of(buffer.begin(), buffer.begin() + 128,
                            [](float sample) { return sample != 0.0f; }));
    EXPECT_TRUE(std::all_of(buffer.begin() + 128, buffer.end(),
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
