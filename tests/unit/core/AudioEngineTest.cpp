#include <gtest/gtest.h>

#include "audio_engine/include/AudioEngine.h"
#include "audio_engine/parameters/ParameterEventQueue.h"
#include "audio_engine/parameters/ParameterManager.h"
#include "audio_engine/voice/VoiceAllocator.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <thread>
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

TEST(AudioEngineTest, QueuedControlStateSurvivesReinitialization) {
    AudioEngine engine;
    engine.initialize(44100.0f);
    engine.getParameterEventQueue()->addEvent("pitch0Hz", 440.0f, 0);

    engine.initialize(48000.0f);
    std::vector<float> block(1);
    engine.processBlock(block.data(), block.size(), 1);

    EXPECT_FLOAT_EQ(
        engine.getParameterManager()->getParameterValue("pitch0Hz"), 440.0f);
}

TEST(AudioEngineTest, CompleteStateSnapshotCrossesOneBoundaryBeforeItsNote) {
    AudioEngine engine;
    engine.initialize(44100.0f);
    engine.setSoftClippingEnabled(false);

    KickParams restored = kDefaultKickParams;
    restored.pitch[0].value = 330.0f;
    restored.membraneLevel = 0.0f;
    restored.transient.impactLevel = 0.0f;
    restored.transient.airLevel = 0.0f;
    restored.sampleLevel = 1.0f;
    restored.outputGain = 1.0f;
    auto layer = std::make_shared<SampleLayerData>();
    layer->sourceSampleRate = 48000.0f;
    layer->samples.assign(32, -0.25f);

    const std::uint64_t revision =
        engine.setStateSnapshot(restored, std::move(layer));
    EXPECT_GT(revision, 0u);
    EXPECT_LT(engine.getAppliedStateRevision(), revision);

    // Hosts may restore state before their final setupProcessing/setActive.
    engine.initialize(48000.0f);
    ASSERT_TRUE(engine.scheduleNoteOnEvent(36, 1.0f, 0));
    std::vector<float> block(8);
    engine.processBlock(block.data(), block.size(), 1);

    EXPECT_EQ(engine.getAppliedStateRevision(), revision);
    EXPECT_FLOAT_EQ(engine.getParams().pitch[0].value, 330.0f);
    EXPECT_FLOAT_EQ(engine.getParams().sampleLevel, 1.0f);
    EXPECT_LT(block.front(), 0.0f);
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

TEST(AudioEngineTest, SampleLayerIsStagedSanitizedAndResampled) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);
    engine.setParameter("membraneLevel", 0.0f);
    engine.setParameter("impactLevel", 0.0f);
    engine.setParameter("airLevel", 0.0f);
    engine.setParameter("sampleLevel", 1.0f);
    engine.setOutputGain(1.0f);

    auto sampleLayer = std::make_shared<SampleLayerData>();
    sampleLayer->sourceSampleRate = 24000.0f;
    sampleLayer->samples.assign(480, 0.25f);
    sampleLayer->samples[0] = std::numeric_limits<float>::quiet_NaN();
    engine.setSampleLayer(sampleLayer);
    const auto installed = engine.getSampleLayer();
    ASSERT_NE(installed, nullptr);
    EXPECT_FLOAT_EQ(installed->sourceSampleRate, 24000.0f);
    EXPECT_FLOAT_EQ(installed->samples[0], 0.0f);
    engine.enqueueNoteOn(36, 1.0f);

    std::vector<float> buffer(8);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    EXPECT_FLOAT_EQ(buffer[0], 0.0f);
    EXPECT_FLOAT_EQ(buffer[1], 0.125f);
    for (std::size_t sample = 2; sample < buffer.size(); ++sample) {
        EXPECT_FLOAT_EQ(buffer[sample], 0.25f);
    }
}

TEST(AudioEngineTest, ClearingSampleLayerAffectsFutureHitsOnly) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);
    engine.setParameter("membraneLevel", 0.0f);
    engine.setParameter("impactLevel", 0.0f);
    engine.setParameter("airLevel", 0.0f);
    engine.setParameter("sampleLevel", 1.0f);
    engine.setOutputGain(1.0f);

    auto sampleLayer = std::make_shared<SampleLayerData>();
    sampleLayer->sourceSampleRate = 48000.0f;
    sampleLayer->samples.assign(480, 0.25f);
    engine.setSampleLayer(sampleLayer);
    engine.noteOn(36, 1.0f);
    engine.clearSampleLayer();
    EXPECT_EQ(engine.getSampleLayer(), nullptr);
    engine.noteOn(37, 1.0f);

    std::vector<float> buffer(8);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    for (const float sample : buffer) {
        EXPECT_FLOAT_EQ(sample, 0.25f);
    }
}

TEST(AudioEngineTest, RetiredSampleLayersLiveThroughActiveHitsThenRelease) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setParameter("sampleLevel", 1.0f);

    auto first = std::make_shared<SampleLayerData>();
    first->sourceSampleRate = 48000.0f;
    first->samples.assign(480, 0.25f);
    engine.setSampleLayer(first);
    engine.noteOn(36, 1.0f);
    auto installedFirst = engine.getSampleLayer();
    std::weak_ptr<const SampleLayerData> retired = installedFirst;
    installedFirst.reset();

    auto second = std::make_shared<SampleLayerData>();
    second->sourceSampleRate = 48000.0f;
    second->samples.assign(480, -0.25f);
    engine.setSampleLayer(second);
    std::vector<float> shortBlock(64);
    engine.processBlock(shortBlock.data(), shortBlock.size(), 1);
    EXPECT_FALSE(retired.expired());

    std::vector<float> remainder(12000);
    engine.processBlock(remainder.data(), remainder.size(), 1);
    (void)engine.getSampleLayer(); // control-thread reclamation point
    EXPECT_TRUE(retired.expired());
}

TEST(AudioEngineTest, IntermediateSampleRevisionsDoNotAccumulateBehindAnActiveHit) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setParameter("sampleLevel", 1.0f);

    auto first = std::make_shared<SampleLayerData>();
    first->sourceSampleRate = 48000.0f;
    first->samples.assign(480, 0.25f);
    engine.setSampleLayer(first);
    engine.noteOn(36, 1.0f);

    std::vector<std::weak_ptr<const SampleLayerData>> intermediate;
    for (int revision = 0; revision < 16; ++revision) {
        auto layer = std::make_shared<SampleLayerData>();
        layer->sourceSampleRate = 48000.0f;
        layer->samples.assign(480, static_cast<float>(revision) / 32.0f);
        engine.setSampleLayer(layer);
        if (revision < 15) {
            auto installed = engine.getSampleLayer();
            intermediate.emplace_back(installed);
        }
    }

    std::vector<float> block(1);
    engine.processBlock(block.data(), block.size(), 1);
    (void)engine.getSampleLayer();
    for (const auto& retired : intermediate) {
        EXPECT_TRUE(retired.expired());
    }
}

TEST(AudioEngineTest, ReplacedSamplesCanRetireBeforeTheFirstAudioCallback) {
    AudioEngine engine;
    engine.initialize(48000.0f);

    std::vector<std::weak_ptr<const SampleLayerData>> retired;
    for (int revision = 0; revision < 16; ++revision) {
        auto layer = std::make_shared<SampleLayerData>();
        layer->sourceSampleRate = 48000.0f;
        layer->samples.assign(64, static_cast<float>(revision) / 32.0f);
        engine.setSampleLayer(layer);
        if (revision < 15) {
            retired.emplace_back(engine.getSampleLayer());
        }
    }
    (void)engine.getSampleLayer();
    for (const auto& replaced : retired) {
        EXPECT_TRUE(replaced.expired());
    }
}

TEST(AudioEngineTest, SampleLayerHandoffSurvivesConcurrentReplacement) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.prepare(32);
    engine.setParameter("sampleLevel", 1.0f);

    std::atomic<bool> start {false};
    std::thread audio([&] {
        std::vector<float> block(32);
        while (!start.load(std::memory_order_acquire)) {
        }
        for (int iteration = 0; iteration < 1000; ++iteration) {
            if (iteration % 25 == 0) {
                engine.noteOn(36, 1.0f);
            }
            engine.processBlock(block.data(), block.size(), 1);
            EXPECT_TRUE(std::all_of(block.begin(), block.end(), [](float sample) {
                return std::isfinite(sample);
            }));
        }
    });

    start.store(true, std::memory_order_release);
    for (int revision = 0; revision < 1000; ++revision) {
        auto layer = std::make_shared<SampleLayerData>();
        layer->sourceSampleRate = 48000.0f;
        layer->samples.assign(64, static_cast<float>(revision % 8) / 8.0f);
        engine.setSampleLayer(std::move(layer));
        (void)engine.getSampleLayer();
    }
    audio.join();
    EXPECT_NE(engine.getSampleLayer(), nullptr);
}

TEST(AudioEngineTest, OutputEqSaturationAndLimiterAreEffective) {
    auto render = [](float lowDb, float midDb, float highDb,
                     float saturation, float ceilingDb) {
        AudioEngine engine;
        engine.initialize(48000.0f);
        engine.setParameter("eqLowDb", lowDb);
        engine.setParameter("eqMidDb", midDb);
        engine.setParameter("eqHighDb", highDb);
        engine.setParameter("saturation", saturation);
        engine.setParameter("limiterCeilingDb", ceilingDb);
        engine.noteOn(36, 1.0f);
        std::vector<float> buffer(4096);
        engine.processBlock(buffer.data(), buffer.size(), 1);
        return buffer;
    };

    const auto neutral = render(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_NE(neutral, render(6.0f, 0.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_NE(neutral, render(0.0f, 6.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_NE(neutral, render(0.0f, 0.0f, 6.0f, 0.0f, 0.0f));
    EXPECT_NE(neutral, render(0.0f, 0.0f, 0.0f, 1.0f, 0.0f));

    const auto limited = render(12.0f, 12.0f, 12.0f, 1.0f, -6.0f);
    const float ceiling = std::pow(10.0f, -6.0f / 20.0f);
    EXPECT_TRUE(std::all_of(limited.begin(), limited.end(), [ceiling](float sample) {
        return std::isfinite(sample) && std::abs(sample) <= ceiling;
    }));
}

TEST(AudioEngineTest, NonFiniteParametersFallBackToAuthoritativeDefaults) {
    AudioEngine engine;
    engine.initialize(std::numeric_limits<float>::infinity());
    engine.setParameter("membraneLevel", std::numeric_limits<float>::quiet_NaN());
    engine.setParameter("phaseDegrees", std::numeric_limits<float>::infinity());
    engine.setParameter("eqLowDb", -std::numeric_limits<float>::infinity());
    engine.setParameter("saturation", std::numeric_limits<float>::quiet_NaN());
    engine.setParameter("limiterCeilingDb", std::numeric_limits<float>::infinity());

    EXPECT_FLOAT_EQ(engine.getSampleRate(), 48000.0f);
    EXPECT_FLOAT_EQ(engine.getParameterManager()->getParameterValue("membraneLevel"),
                    kDefaultKickParams.membraneLevel);
    EXPECT_FLOAT_EQ(engine.getParameterManager()->getParameterValue("phaseDegrees"),
                    kDefaultKickParams.phaseDegrees);
    EXPECT_FLOAT_EQ(engine.getParameterManager()->getParameterValue("eqLowDb"),
                    kDefaultKickParams.outputStage.eqLowDb);
    EXPECT_FLOAT_EQ(engine.getParameterManager()->getParameterValue("saturation"),
                    kDefaultKickParams.outputStage.saturation);
    EXPECT_FLOAT_EQ(
        engine.getParameterManager()->getParameterValue("limiterCeilingDb"),
        kDefaultKickParams.outputStage.limiterCeilingDb);

    engine.noteOn(36, 1.0f);
    std::vector<float> buffer(1024);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(), [](float sample) {
        return std::isfinite(sample);
    }));
}

TEST(AudioEngineTest, TypedNoteEventStartsAtExactSampleOffset) {
    AudioEngine reference;
    reference.initialize(48000.0f);
    reference.setSoftClippingEnabled(false);
    reference.noteOn(36, 1.0f);
    std::vector<float> hit(512);
    reference.processBlock(hit.data(), hit.size(), 1);

    AudioEngine scheduled;
    scheduled.initialize(48000.0f);
    scheduled.setSoftClippingEnabled(false);
    ASSERT_TRUE(scheduled.scheduleNoteOnEvent(36, 1.0f, 137));
    std::vector<float> rendered(512);
    scheduled.processBlock(rendered.data(), rendered.size(), 1);

    EXPECT_TRUE(std::all_of(rendered.begin(), rendered.begin() + 137,
                            [](float sample) { return sample == 0.0f; }));
    EXPECT_TRUE(std::equal(rendered.begin() + 137, rendered.end(), hit.begin()));
}

TEST(AudioEngineTest, TypedParametersApplyBeforeNotesAtTheSameOffset) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);

    // Schedule the note first to prove event type, not arrival order, controls
    // precedence at a shared sample position.
    ASSERT_TRUE(engine.scheduleNoteOnEvent(36, 1.0f, 73));
    ASSERT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::OutputGain, 0.0f, 73));
    std::vector<float> buffer(256);
    engine.processBlock(buffer.data(), buffer.size(), 1);

    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(),
                            [](float sample) { return sample == 0.0f; }));
    ASSERT_EQ(engine.getVoiceAllocator()->getNumActiveVoices(), 1);
    EXPECT_FLOAT_EQ(engine.getVoiceAllocator()->getVoice(0).getParams().outputGain,
                    0.0f);
}

TEST(AudioEngineTest, TypedParameterPointsAreSortedAndEveryPointIsApplied) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);

    // Host parameter queues are grouped by parameter, so arrival order is not
    // necessarily timeline order across all host queues.
    ASSERT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::Pitch0Hz, 400.0f, 40));
    ASSERT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::Pitch0Hz, 300.0f, 20));
    ASSERT_TRUE(engine.scheduleNoteOnEvent(37, 1.0f, 40));
    ASSERT_TRUE(engine.scheduleNoteOnEvent(36, 1.0f, 20));

    std::vector<float> buffer(64);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    ASSERT_EQ(engine.getVoiceAllocator()->getNumActiveVoices(), 2);
    EXPECT_FLOAT_EQ(
        engine.getVoiceAllocator()->getVoice(0).getParams().pitch[0].value,
        300.0f);
    EXPECT_FLOAT_EQ(
        engine.getVoiceAllocator()->getVoice(1).getParams().pitch[0].value,
        400.0f);
    EXPECT_FLOAT_EQ(
        engine.getParameterManager()->getParameterValue("pitch0Hz"), 400.0f);
}

TEST(AudioEngineTest, LaterUiTimelinePointWinsFinalStateOverEarlierHostPoint) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.getParameterEventQueue()->addEvent("pitch0Hz", 500.0f, 50);
    ASSERT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::Pitch0Hz, 300.0f, 20));

    std::vector<float> buffer(64);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    EXPECT_FLOAT_EQ(
        engine.getParameterManager()->getParameterValue("pitch0Hz"), 500.0f);
}

TEST(AudioEngineTest, SimultaneousTrajectoryMovesAreCommittedAsOneState) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setParameter("pitch1TimeMs", 90.0f);
    engine.setParameter("pitch2TimeMs", 100.0f);

    // VST queues are grouped by parameter and may arrive in either order.
    // Applying the later point first must not let intermediate sanitization
    // push it past the final requested earlier point.
    ASSERT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::Pitch2TimeMs, 20.0f, 64));
    ASSERT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::Pitch1TimeMs, 10.0f, 64));
    ASSERT_TRUE(engine.scheduleNoteOnEvent(36, 1.0f, 64));

    std::vector<float> buffer(128);
    engine.processBlock(buffer.data(), buffer.size(), 1);

    ASSERT_EQ(engine.getVoiceAllocator()->getNumActiveVoices(), 1);
    const auto& triggered = engine.getVoiceAllocator()->getVoice(0).getParams();
    EXPECT_FLOAT_EQ(triggered.pitch[1].timeMs, 10.0f);
    EXPECT_FLOAT_EQ(triggered.pitch[2].timeMs, 20.0f);
}

TEST(AudioEngineTest, TypedNotesMergeWithUiParameterQueue) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);
    ASSERT_TRUE(engine.scheduleNoteOnEvent(36, 1.0f, 64));
    engine.getParameterEventQueue()->addEvent("outputGain", 0.0f, 64);

    std::vector<float> buffer(256);
    engine.processBlock(buffer.data(), buffer.size(), 1);
    EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(),
                            [](float sample) { return sample == 0.0f; }));
}

TEST(AudioEngineTest, BlockEndParametersPrecedeNotesAndAffectNextBlock) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    engine.setSoftClippingEnabled(false);
    constexpr std::uint32_t blockSamples = 256;

    ASSERT_TRUE(engine.scheduleNoteOnEvent(36, 1.0f, blockSamples));
    ASSERT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::OutputGain, 0.0f, blockSamples));
    std::vector<float> first(blockSamples);
    engine.processBlock(first.data(), first.size(), 1);

    EXPECT_TRUE(std::all_of(first.begin(), first.end(),
                            [](float sample) { return sample == 0.0f; }));
    ASSERT_EQ(engine.getVoiceAllocator()->getNumActiveVoices(), 1);
    EXPECT_FLOAT_EQ(engine.getVoiceAllocator()->getVoice(0).getParams().outputGain,
                    0.0f);

    std::vector<float> second(blockSamples);
    engine.processBlock(second.data(), second.size(), 1);
    EXPECT_TRUE(std::all_of(second.begin(), second.end(),
                            [](float sample) { return sample == 0.0f; }));
}

TEST(AudioEngineTest, TypedRealtimeQueuesHaveFixedCapacityAndCanBeCleared) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    for (std::size_t index = 0;
         index < AudioEngine::kMaxRealtimeParameterEvents; ++index) {
        ASSERT_TRUE(engine.scheduleParameterEvent(
            KickParameterId::Pitch0Hz, 220.0f, 0));
    }
    EXPECT_FALSE(engine.scheduleParameterEvent(
        KickParameterId::Pitch0Hz, 220.0f, 0));

    for (std::size_t index = 0;
         index < AudioEngine::kMaxRealtimeNoteEvents; ++index) {
        ASSERT_TRUE(engine.scheduleNoteOnEvent(
            36, 1.0f, static_cast<std::uint32_t>(index)));
    }
    EXPECT_FALSE(engine.scheduleNoteOnEvent(36, 1.0f, 0));

    engine.clearScheduledEvents();
    EXPECT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::Pitch0Hz, 330.0f, 0));
    EXPECT_TRUE(engine.scheduleNoteOnEvent(36, 1.0f, 0));
}

TEST(AudioEngineTest, ParameterOverflowStillPreservesLatestBlockState) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    for (std::size_t index = 0;
         index < AudioEngine::kMaxRealtimeParameterEvents; ++index) {
        ASSERT_TRUE(engine.scheduleParameterEvent(
            KickParameterId::Pitch0Hz, 220.0f, 0));
    }
    EXPECT_FALSE(engine.scheduleParameterEvent(
        KickParameterId::Pitch0Hz, 777.0f, 0));

    std::vector<float> block(1);
    engine.processBlock(block.data(), block.size(), 1);
    EXPECT_FLOAT_EQ(
        engine.getParameterManager()->getParameterValue("pitch0Hz"), 777.0f);
}

TEST(AudioEngineTest, ZeroLengthFlushAppliesParametersBeforeNotes) {
    AudioEngine engine;
    engine.initialize(48000.0f);
    ASSERT_TRUE(engine.scheduleNoteOnEvent(36, 1.0f, 99));
    ASSERT_TRUE(engine.scheduleParameterEvent(
        KickParameterId::OutputGain, 0.0f, 42));
    engine.flushScheduledEvents();

    ASSERT_EQ(engine.getVoiceAllocator()->getNumActiveVoices(), 1);
    EXPECT_FLOAT_EQ(engine.getVoiceAllocator()->getVoice(0).getParams().outputGain,
                    0.0f);
    EXPECT_FLOAT_EQ(engine.getParameterManager()->getParameterValue("outputGain"),
                    0.0f);
}
