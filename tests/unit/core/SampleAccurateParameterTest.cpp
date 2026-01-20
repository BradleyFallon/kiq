#include <gtest/gtest.h>
#include "../../../src/audio_engine/include/AudioEngine.h"
#include "../../../src/audio_engine/parameters/ParameterEventQueue.h"
#include "../../../src/audio_engine/parameters/ParameterManager.h"
#include <vector>
#include <cmath>

using namespace KickDrum;

class SampleAccurateParameterTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine.initialize(48000.0f);
    }
    
    AudioEngine engine;
};

// Test: Parameter event queue is accessible
TEST_F(SampleAccurateParameterTest, EventQueueAccessible) {
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    ASSERT_NE(queue, nullptr);
    EXPECT_TRUE(queue->isEmpty());
}

// Test: setParameter schedules immediate event
TEST_F(SampleAccurateParameterTest, SetParameterSchedulesEvent) {
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    ASSERT_NE(queue, nullptr);
    
    // Set a parameter
    engine.setParameter("basePitch", 60.0f);
    
    // Event should be queued
    EXPECT_EQ(queue->getEventCount(), 1);
    
    // Process a buffer to consume the event
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // Event should be consumed
    EXPECT_EQ(queue->getEventCount(), 0);
    
    // Parameter should be updated in manager
    ParameterManager* paramManager = engine.getParameterManager();
    ASSERT_NE(paramManager, nullptr);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("basePitch"), 60.0f);
}

// Test: Multiple events in single buffer
TEST_F(SampleAccurateParameterTest, MultipleEventsInBuffer) {
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    ASSERT_NE(queue, nullptr);
    
    // Add multiple events at different sample offsets
    queue->addEvent("basePitch", 50.0f, 0);
    queue->addEvent("sineLevel", 80.0f, 128);
    queue->addEvent("harmonicRatio", 3.0f, 256);
    
    EXPECT_EQ(queue->getEventCount(), 3);
    
    // Trigger a note to generate audio
    engine.noteOn(60, 1.0f);
    
    // Process buffer
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // All events should be consumed
    EXPECT_EQ(queue->getEventCount(), 0);
    
    // Parameters should be updated
    ParameterManager* paramManager = engine.getParameterManager();
    ASSERT_NE(paramManager, nullptr);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("basePitch"), 50.0f);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("sineLevel"), 80.0f);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("harmonicRatio"), 3.0f);
}

// Test: Events are processed in order
TEST_F(SampleAccurateParameterTest, EventsProcessedInOrder) {
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    ASSERT_NE(queue, nullptr);
    
    // Add events in non-sorted order
    queue->addEvent("basePitch", 100.0f, 256);
    queue->addEvent("basePitch", 50.0f, 0);
    queue->addEvent("basePitch", 75.0f, 128);
    
    // Process buffer
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // Final value should be from the last event (at offset 256)
    ParameterManager* paramManager = engine.getParameterManager();
    ASSERT_NE(paramManager, nullptr);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("basePitch"), 100.0f);
}

// Test: Events beyond buffer size are clamped
TEST_F(SampleAccurateParameterTest, EventsBeyondBufferClamped) {
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    ASSERT_NE(queue, nullptr);
    
    // Add event beyond buffer size
    queue->addEvent("basePitch", 60.0f, 1000);  // Buffer is only 512 samples
    
    // Process buffer
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // Event should still be processed (clamped to buffer end)
    EXPECT_EQ(queue->getEventCount(), 0);
    
    ParameterManager* paramManager = engine.getParameterManager();
    ASSERT_NE(paramManager, nullptr);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("basePitch"), 60.0f);
}

// Test: No events - fast path
TEST_F(SampleAccurateParameterTest, NoEventsFastPath) {
    // Trigger a note
    engine.noteOn(60, 1.0f);
    
    // Process buffer with no events
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // Should produce audio (non-zero samples)
    bool hasAudio = false;
    for (float sample : buffer) {
        if (std::abs(sample) > 0.0001f) {
            hasAudio = true;
            break;
        }
    }
    EXPECT_TRUE(hasAudio);
}

// Test: Parameter changes affect audio output
TEST_F(SampleAccurateParameterTest, ParameterChangesAffectAudio) {
    // Trigger a note
    engine.noteOn(60, 1.0f);
    
    // Process buffer with default parameters
    std::vector<float> buffer1(512, 0.0f);
    engine.processBlock(buffer1.data(), 512, 1);
    
    // Calculate RMS of first buffer
    float rms1 = 0.0f;
    for (float sample : buffer1) {
        rms1 += sample * sample;
    }
    rms1 = std::sqrt(rms1 / buffer1.size());
    
    // Trigger another note
    engine.noteOn(60, 1.0f);
    
    // Change sine level to 0 (should reduce output)
    engine.setParameter("sineLevel", 0.0f);
    
    // Process buffer with modified parameters
    std::vector<float> buffer2(512, 0.0f);
    engine.processBlock(buffer2.data(), 512, 1);
    
    // Calculate RMS of second buffer
    float rms2 = 0.0f;
    for (float sample : buffer2) {
        rms2 += sample * sample;
    }
    rms2 = std::sqrt(rms2 / buffer2.size());
    
    // Second buffer should have lower RMS (sine level reduced)
    EXPECT_LT(rms2, rms1);
}

// Test: Envelope parameters can be updated
TEST_F(SampleAccurateParameterTest, EnvelopeParametersUpdated) {
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    ASSERT_NE(queue, nullptr);
    
    // Update envelope parameters
    queue->addEvent("attack", 10.0f, 0);      // 10ms attack
    queue->addEvent("decay", 200.0f, 0);      // 200ms decay
    queue->addEvent("sustain", 50.0f, 0);     // 50% sustain
    queue->addEvent("release", 50.0f, 0);     // 50ms release
    
    // Process buffer
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // Parameters should be updated
    ParameterManager* paramManager = engine.getParameterManager();
    ASSERT_NE(paramManager, nullptr);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("attack"), 10.0f);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("decay"), 200.0f);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("sustain"), 50.0f);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("release"), 50.0f);
}

// Test: Effects parameters can be updated
TEST_F(SampleAccurateParameterTest, EffectsParametersUpdated) {
    ParameterEventQueue* queue = engine.getParameterEventQueue();
    ASSERT_NE(queue, nullptr);
    
    // Update effects parameters
    queue->addEvent("compressorThreshold", -20.0f, 0);
    queue->addEvent("compressorRatio", 8.0f, 0);
    queue->addEvent("reverbRoomSize", 75.0f, 0);
    queue->addEvent("reverbMix", 30.0f, 0);
    
    // Process buffer
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // Parameters should be updated
    ParameterManager* paramManager = engine.getParameterManager();
    ASSERT_NE(paramManager, nullptr);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("compressorThreshold"), -20.0f);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("compressorRatio"), 8.0f);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("reverbRoomSize"), 75.0f);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("reverbMix"), 30.0f);
}

// Test: Master level parameter
TEST_F(SampleAccurateParameterTest, MasterLevelParameter) {
    // Trigger a note
    engine.noteOn(60, 1.0f);
    
    // Set master level to 50%
    engine.setParameter("masterLevel", 50.0f);
    
    // Process buffer
    std::vector<float> buffer(512, 0.0f);
    engine.processBlock(buffer.data(), 512, 1);
    
    // Master level should be updated
    ParameterManager* paramManager = engine.getParameterManager();
    ASSERT_NE(paramManager, nullptr);
    EXPECT_FLOAT_EQ(paramManager->getParameterValue("masterLevel"), 50.0f);
}
